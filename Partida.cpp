#include "stdafx.h"
#include "Partida.h"


Partida::Partida()
{
	tabuleiro = gcnew array<int>(9);
	array<int>^ temp = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	tabuleiro->Copy(temp, tabuleiro, 9);

	jogadores = gcnew List<NetworkStream^>();
	espectadores = gcnew List<NetworkStream^>();
}


Partida::~Partida()
{
}
