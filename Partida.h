#pragma once

using namespace System;
using namespace System::Text;
using namespace System::Threading;
using namespace System::Net;
using namespace System::Net::Sockets;
using namespace System::IO;
using namespace System::Collections;

public ref class Partida
{
public:
	Partida();
	~Partida();

	array<int>^ tabuleiro;
	array<NetworkStream^>^ jogadores;
};

