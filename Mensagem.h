#pragma once

using namespace System;
using namespace System::Text;
using namespace System::Threading;


public ref class Mensagem
{
public:
	Mensagem();
	~Mensagem();

	String^ nomeFuncao;
	String^ parametro;
};

