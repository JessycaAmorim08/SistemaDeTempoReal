
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "PCIE.h"
#include <time.h>
#include <opencv2/highgui/highgui.hpp>

using namespace cv;


#define DEMO_PCIE_USER_BAR			PCIE_BAR0
#define DEMO_PCIE_IO_LED_ADDR		0x00
#define DEMO_PCIE_IO_BUTTON_ADDR	0x20
#define DEMO_PCIE_FIFO_WRITE_ADDR	0x40
#define DEMO_PCIE_FIFO_STATUS_ADDR	0x60
#define DEMO_PCIE_FIFO_READ_ADDR	0x80
#define DEMO_PCIE_MEM_ADDR			0x20000

#define MEM_SIZE			(128*1024) //128KB
#define FIFO_SIZE			(16*1024) // 2KBx8

Mat verde;
Mat azul;
Mat vermelho;
Mat amarelo;
Mat desligado;
Mat tela;

typedef enum{
	MENU_LED = 0,
	MENU_BUTTON,
	MENU_DMA_MEMORY,
	MENU_DMA_FIFO,
	MENU_GENIUS,
	MENU_QUIT = 99
}MENU_ID;

void UI_ShowMenu(void){
	printf("==============================\r\n");
	printf("[%d]: Led control\r\n", MENU_LED);
	printf("[%d]: Button Status Read\r\n", MENU_BUTTON);
	printf("[%d]: DMA Memory Test\r\n", MENU_DMA_MEMORY);
	printf("[%d]: DMA FifoTest\r\n", MENU_DMA_FIFO);
	printf("[%d]: Genius\r\n", MENU_GENIUS);
	printf("[%d]: Quit\r\n", MENU_QUIT);
	printf("Plesae input your selection:");
}

int UI_UserSelect(void){
	int nSel;
	scanf("%d",&nSel);
	return nSel;
}


BOOL TEST_LED(PCIE_HANDLE hPCIe){
	BOOL bPass;
	int	Mask;
	
	
	scanf("%d", &Mask);

	bPass = PCIE_Write32(hPCIe, DEMO_PCIE_USER_BAR, DEMO_PCIE_IO_LED_ADDR,(DWORD)Mask);
	if (bPass)
		printf("Led control success, mask=%xh\r\n", Mask);
	else
		printf("Led conrol failed\r\n");

	
	return bPass;
}

BOOL TEST_BUTTON(PCIE_HANDLE hPCIe){
	BOOL bPass = TRUE;
	DWORD Status;

	bPass = PCIE_Read32(hPCIe, DEMO_PCIE_USER_BAR, DEMO_PCIE_IO_BUTTON_ADDR,&Status);
	if (bPass)
		printf("Button status mask:=%xh\r\n", Status);
	else
		printf("Failed to read button status\r\n");

	
	return bPass;
}

char PAT_GEN(int nIndex){
	char Data;
	Data = nIndex & 0xFF;
	return Data;
}

BOOL TEST_DMA_MEMORY(PCIE_HANDLE hPCIe){
	BOOL bPass;
	int i;
	const int nTestSize = MEM_SIZE;
	const PCIE_LOCAL_ADDRESS LocalAddr = DEMO_PCIE_MEM_ADDR;
	char *pWrite;
	char *pRead;
	char szError[256];


	pWrite = (char *)malloc(nTestSize);
	pRead = (char *)malloc(nTestSize);
	if (!pWrite || !pRead){
		bPass = FALSE;
		sprintf(szError, "DMA Memory:malloc failed\r\n");
	}
	

	// init test pattern
	for(i=0;i<nTestSize && bPass;i++)
		*(pWrite+i) = PAT_GEN(i);

	// write test pattern
	if (bPass){
		bPass = PCIE_DmaWrite(hPCIe, LocalAddr, pWrite, nTestSize);
		if (!bPass)
			sprintf(szError, "DMA Memory:PCIE_DmaWrite failed\r\n");
	}		

	// read back test pattern and verify
	if (bPass){
		bPass = PCIE_DmaRead(hPCIe, LocalAddr, pRead, nTestSize);

		if (!bPass){
			sprintf(szError, "DMA Memory:PCIE_DmaRead failed\r\n");
		}else{
			for(i=0;i<nTestSize && bPass;i++){
				if (*(pRead+i) != PAT_GEN(i)){
					bPass = FALSE;
					sprintf(szError, "DMA Memory:Read-back verify unmatch, index = %d, read=%xh, expected=%xh\r\n", i, *(pRead+i), PAT_GEN(i));
				}
			}
		}
	}


	// free resource
	if (pWrite)
		free(pWrite);
	if (pRead)
		free(pRead);
	
	if (!bPass)
		printf("%s", szError);
	else
		printf("DMA-Memory (Size = %d byes) pass\r\n", nTestSize);


	return bPass;
}

BOOL TEST_DMA_FIFO(PCIE_HANDLE hPCIe){
	BOOL bPass;
	int i;
	const int nTestSize = FIFO_SIZE;
	const PCIE_LOCAL_ADDRESS FifoID_Write = DEMO_PCIE_FIFO_WRITE_ADDR;
	const PCIE_LOCAL_ADDRESS FifoID_Read = DEMO_PCIE_FIFO_READ_ADDR;
	char *pBuff;
	char szError[256];


	pBuff = (char *)malloc(nTestSize);
	if (!pBuff){
		bPass = FALSE;
		sprintf(szError, "DMA Fifo: malloc failed\r\n");
	}
	

	// init test pattern
	if (bPass){
		for(i=0;i<nTestSize;i++)
			*(pBuff+i) = PAT_GEN(i);
	}

	// write test pattern into fifo
	if (bPass){
		bPass = PCIE_DmaFifoWrite(hPCIe, FifoID_Write, pBuff, nTestSize);
		if (!bPass)
			sprintf(szError, "DMA Fifo: PCIE_DmaFifoWrite failed\r\n");
	}		

	// read back test pattern and verify
	if (bPass){
		memset(pBuff, 0, nTestSize); // reset buffer content
		bPass = PCIE_DmaFifoRead(hPCIe, FifoID_Read, pBuff, nTestSize);

		if (!bPass){
			sprintf(szError, "DMA Fifo: PCIE_DmaFifoRead failed\r\n");
		}else{
			for(i=0;i<nTestSize && bPass;i++){
				if (*(pBuff+i) != PAT_GEN(i)){
					bPass = FALSE;
					sprintf(szError, "DMA Fifo: Read-back verify unmatch, index = %d, read=%xh, expected=%xh\r\n", i, *(pBuff+i), PAT_GEN(i));
				}
			}
		}
	}


	// free resource
	if (pBuff)
		free(pBuff);
	
	if (!bPass)
		printf("%s", szError);
	else
		printf("DMA-Fifo (Size = %d byes) pass\r\n", nTestSize);


	return bPass;
}

void imagem(DWORD mascara, int score){
	switch (mascara){
	case 0x7:
		vermelho.copyTo(tela);
		break;
	case 0xB:
		verde.copyTo(tela);
		break;
	case 0xD:
		azul.copyTo(tela);
		break;
	case 0xE:
		amarelo.copyTo(tela);
		break;
	default:
		desligado.copyTo(tela);

	}
	char pontuacao[5];
	sprintf(pontuacao, "%02d", score);
	putText(tela, pontuacao, Point((tela.cols / 2) - 50, (desligado.rows / 2) + 50), 0, 3, Scalar(0, 0, 255));
	imshow("Genius Game", tela);
	waitKey(500);
	
}

void launchGenius(PCIE_HANDLE hPCIe){
	DWORD lista[100];
	imagem(-1,0);
	for( int cont = 0; cont < 100; cont++){
		
		switch (rand()%4){
			case 0:
				lista[cont] = 0x7;
				break;
			case 1:
				lista[cont] = 0xB;
				break;
			case 2:
				lista[cont] = 0xD;
				break;
			case 3:
				lista[cont] = 0xE;
				break;
		}
		for (int j = 0; j <= cont; j++){
			//Ler todos os elmentos do 0 até cont
			PCIE_Write32(hPCIe, DEMO_PCIE_USER_BAR, DEMO_PCIE_IO_LED_ADDR, (DWORD)lista[j] ^ 0xf);
			imagem((DWORD)lista[j],cont);
			//Acender os leds nessa ordem
			PCIE_Write32(hPCIe, DEMO_PCIE_USER_BAR, DEMO_PCIE_IO_LED_ADDR, (DWORD)0);
			imagem(-1, cont);
		}
		
		PCIE_Write32(hPCIe, DEMO_PCIE_USER_BAR, DEMO_PCIE_IO_LED_ADDR, (DWORD)0000);
		for (int j = 0; j <= cont; j++){
			DWORD Status;
			//Ler todos os botoes pressionados
			do{
				PCIE_Read32(hPCIe, DEMO_PCIE_USER_BAR, DEMO_PCIE_IO_BUTTON_ADDR, &Status);
			} while (Status == 0xF);
			PCIE_Write32(hPCIe, DEMO_PCIE_USER_BAR, DEMO_PCIE_IO_LED_ADDR, (DWORD)Status^0xf);
			imagem(Status,cont);
			PCIE_Write32(hPCIe, DEMO_PCIE_USER_BAR, DEMO_PCIE_IO_LED_ADDR, (DWORD)0);
			imagem(-1, cont);
			//Se a sequencia estiver errada sai.
			if (Status != lista[j]){
				printf("Você perdeu!");
				imagem(-1, cont);
				return ;
			}
			waitKey(500);
			
		}
		printf("Score: %i \n", cont + 1);
	}

}



int main(void)
{
	void *lib_handle;
	PCIE_HANDLE hPCIE;
	BOOL bQuit = FALSE;
	int nSel;
	srand(time(NULL));
	verde = imread("Verde.png");
	azul = imread("Azul.png");
	vermelho = imread("Vermelho.png");
	amarelo = imread("Amarelo.png");
	desligado = imread("Desligado.png");
	tela = imread("Desligado.png");
	
	printf("== Terasic: PCIe Demo Program ==\r\n");

	lib_handle = PCIE_Load();
	if (!lib_handle){
		printf("PCIE_Load failed!\r\n");
		return 0;
	}

	hPCIE = PCIE_Open(DEFAULT_PCIE_VID,DEFAULT_PCIE_DID,0);
	if (!hPCIE){
		printf("PCIE_Open failed\r\n");
	}else{
		while(!bQuit){
			UI_ShowMenu();
			nSel = UI_UserSelect();
			switch(nSel){	
				case MENU_LED:
					TEST_LED(hPCIE);
					break;
				case MENU_BUTTON:
					TEST_BUTTON(hPCIE);
					break;
				case MENU_DMA_MEMORY:
					TEST_DMA_MEMORY(hPCIE);
					break;
				case MENU_DMA_FIFO:
					TEST_DMA_FIFO(hPCIE);
					break;
				case MENU_GENIUS:
					launchGenius(hPCIE);
					break;
				case MENU_QUIT:
					bQuit = TRUE;
					printf("Bye!\r\n");
					break;
				default:
					printf("Invalid selection\r\n");
			} // switch

		}// while
		PCIE_Close(hPCIE);

	}


	PCIE_Unload(lib_handle);
	return 0;
}

