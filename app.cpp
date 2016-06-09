#include "PCIE.h"
#include <time.h>
#include <opencv2/highgui/highgui.hpp>
cv::Mat tela;
void imagem(PCIE_HANDLE hPCIe, DWORD mascaraPCI, DWORD mascara, int score){
	PCIE_Write32(hPCIe, PCIE_BAR0, 0x00, mascaraPCI);
	tela = cv::imread(mascara == 7 ? "1.png" : (mascara == 11 ? "2.png" : (mascara == 13 ? "3.png" : mascara == 14 ? "4.png" : "0.png")));
	putText(tela, (score == -1 ? "FIM" : (score == -2 ? "UIN" : std::to_string(score))), cv::Point((tela.cols / 2) - 50, (tela.rows / 2) + 50), 0, 3, cv::Scalar(0, 0, 255));
	imshow("Genius Game", tela);
	cv::waitKey(500);
}
void launchGenius(PCIE_HANDLE hPCIe){
	DWORD lista[101];
	imagem(hPCIe, (DWORD)0,0, 0);
	for( int cont = 0; cont < 100; cont++){		
		lista[cont] = (0x1 << (rand() % 4))^0xf;
		for (int j = 0; j <= cont; j++){
			imagem(hPCIe, (DWORD)lista[j] ^ 0xf, (DWORD)lista[j], cont);
			imagem(hPCIe, (DWORD) 0, 0, cont);
		}		
		for (int j = 0; j <= cont; j++){
			do{
				PCIE_Read32(hPCIe, PCIE_BAR0, 0x20, &lista[100]);
			} while (lista[100] == 0xF);
			imagem(hPCIe, (DWORD)lista[100] ^ 0xf, lista[100], cont);
			imagem(hPCIe, (DWORD)0, 0, cont);
			if (lista[100] != lista[j]){
				imagem(hPCIe, (DWORD)0,0, -1);
				return ;
			}		
		}
	}
	imagem(hPCIe, (DWORD)0,0, -2);
}
void main(void){
	PCIE_HANDLE hPCIE;
	srand(time(NULL));
	if (!PCIE_Load()){
		return ;
	}
	hPCIE = PCIE_Open(DEFAULT_PCIE_VID,DEFAULT_PCIE_DID,0);
	while (true){
		launchGenius(hPCIE);
	}// while
}