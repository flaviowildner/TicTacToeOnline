#pragma once

using namespace System;
using namespace System::Text;
using namespace System::Threading;
using namespace System::Net;
using namespace System::Net::Sockets;
using namespace System::IO;
using namespace System::Collections;
using namespace System::Collections::Generic;

public ref class Partida
{
public:
	Partida();
	~Partida();

	array<int>^ tabuleiro;
	List<NetworkStream^>^ jogadores;
	List<NetworkStream^>^ espectadores;
};

