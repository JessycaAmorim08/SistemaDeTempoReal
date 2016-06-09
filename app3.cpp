#include <stdio.h>
#include <stdlib.h>
#include "PCIE.h"
#include <time.h>
#include <opencv2/highgui/highgui.hpp>
using namespace cv;
Mat verde,azul,vermelho,amarelo,desligado,tela;
void imagem(DWORD mascara, int score){
	Mat cores[0xf] = { desligado, desligado, desligado, desligado,desligado, desligado, desligado, vermelho,desligado, desligado, desligado, verde, desligado, azul, amarelo};
	cores[mascara].copyTo(tela);
	char pontuacao[5];
	score == -1 ? sprintf(pontuacao, "FIM") : sprintf(pontuacao, "%02d", score);//sprintf(pontuacao, "%02d", score);
	putText(tela, pontuacao, Point((tela.cols / 2) - 50, (desligado.rows / 2) + 50), 0, 3, Scalar(0, 0, 255));
	imshow("Genius Game", tela);
	waitKey(500);	
}
void launchGenius(PCIE_HANDLE hPCIe){
	DWORD lista[100];
	imagem(0,0);
	for( int cont = 0; cont < 5; cont++){		
		lista[cont] = (0x1 << (rand() % 4))^0xf;
		for (int j = 0; j <= cont; j++){
			PCIE_Write32(hPCIe, PCIE_BAR0, 0x00, (DWORD)lista[j] ^ 0xf);
			imagem((DWORD)lista[j],cont);
			PCIE_Write32(hPCIe, PCIE_BAR0, 0x00, (DWORD)0);
			imagem(0, cont);
		}		
		PCIE_Write32(hPCIe, PCIE_BAR0, 0x00, (DWORD)0000);
		for (int j = 0; j <= cont; j++){
			DWORD Status;
			do{
				PCIE_Read32(hPCIe, PCIE_BAR0, 0x20, &Status);
			} while (Status == 0xF);
			PCIE_Write32(hPCIe, PCIE_BAR0, 0x00, (DWORD)Status ^ 0xf);
			imagem(Status,cont);
			PCIE_Write32(hPCIe, PCIE_BAR0, 0x00, (DWORD)0);
			imagem(0, cont);
			if (Status != lista[j]){
				imagem(0, -1);
				return ;
			}		
		}
	}
}
int main(void){
	void *lib_handle;
	PCIE_HANDLE hPCIE;
	srand(time(NULL));
	verde = imread("Verde.png");
	azul = imread("Azul.png");
	vermelho = imread("Vermelho.png");
	amarelo = imread("Amarelo.png");
	desligado = imread("Desligado.png");
	tela = imread("Desligado.png");
	lib_handle = PCIE_Load();
	if (!lib_handle){
		printf("PCIE_Load failed!\r\n");
		return 0;
	}
	hPCIE = PCIE_Open(DEFAULT_PCIE_VID,DEFAULT_PCIE_DID,0);
	while (true){
		launchGenius(hPCIE);
	}// while
	PCIE_Close(hPCIE);
	PCIE_Unload(lib_handle);
	return 0;
}

