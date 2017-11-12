// TicTacToeOnline.cpp : main project file.

#include "stdafx.h"
#include "Partida.h"
#include "Mensagem.h"

using namespace System;
using namespace System::Text;
using namespace System::Threading;
using namespace System::Net;
using namespace System::Net::Sockets;
using namespace System::IO;
using namespace System::Xml;
using namespace System::Xml::Serialization;
using namespace System::Collections;

using namespace std;

#define BUFFER_SIZE 512


String^ imprimirChar(int i) {
	switch (i) {
	case -1:
		return "X";
	case 0:
		return " ";
	case 1:
		return "O";
	}
}

void imprimirTabuleiro(array<int>^ tabuleiro) {
	Console::Clear();
	Console::WriteLine("|" + imprimirChar(tabuleiro[0]) + "|" + imprimirChar(tabuleiro[1]) + "|" + imprimirChar(tabuleiro[2]) + "|");
	Console::WriteLine(" - - -");
	Console::WriteLine("|" + imprimirChar(tabuleiro[3]) + "|" + imprimirChar(tabuleiro[4]) + "|" + imprimirChar(tabuleiro[5]) + "|");
	Console::WriteLine(" - - -");
	Console::WriteLine("|" + imprimirChar(tabuleiro[6]) + "|" + imprimirChar(tabuleiro[7]) + "|" + imprimirChar(tabuleiro[8]) + "|");
	Console::WriteLine(" - - -");
}

int checarVitoria(array<int>^ tabuleiro) {
	static int wins[8][3] = { { 0,1,2 },{ 3,4,5 },{ 6,7,8 },{ 0,3,6 },{ 1,4,7 },{ 2,5,8 },{ 0,4,8 },{ 2,4,6 } };
	for (int i = 0; i<8; i++) {
		if (tabuleiro[wins[i][0]] != 0 && tabuleiro[wins[i][0]] == tabuleiro[wins[i][1]] && tabuleiro[wins[i][0]] == tabuleiro[wins[i][2]])
			return tabuleiro[wins[i][0]];
	}
	return 0;
}


Mensagem^ deserializeMensagem(String^ mensagem) {
	StringReader^ stringReader;

	stringReader = gcnew StringReader(mensagem);
	XmlSerializer^ serializer = gcnew XmlSerializer(Mensagem::typeid);
	return dynamic_cast<Mensagem^>(serializer->Deserialize(stringReader));
}

String^ SerializeMensagem(Mensagem^ mensagem) {
	XmlSerializer^ serializer;
	StringWriter^ stringWriter;

	//Serializando Mensagem a ser enviada
	serializer = gcnew XmlSerializer(Mensagem::typeid);
	stringWriter = gcnew StringWriter();
	serializer->Serialize(stringWriter, mensagem);
	//
	return stringWriter->ToString();
}

String^ receiveDataNetwork(NetworkStream^ stream) {
	array<Byte>^ bytes = gcnew array<Byte>(BUFFER_SIZE);
	StringBuilder^ retorno = gcnew StringBuilder();
	Int32 sizeOfNextMessage, i;


	////Recebendo tamanho da próxima Mensagem
	i = stream->Read(bytes, 0, 4);
	sizeOfNextMessage = BitConverter::ToInt32(bytes, 0);
	////

	////Recebendo tabuleiro
	do
	{
		if (sizeOfNextMessage > bytes->Length) {
			i = stream->Read(bytes, 0, bytes->Length);
		}
		else {
			i = stream->Read(bytes, 0, sizeOfNextMessage);
		}
		sizeOfNextMessage -= bytes->Length;
		retorno->Append(Encoding::ASCII->GetString(bytes, 0, i));
	} while (sizeOfNextMessage > 0);
	////

	return retorno->ToString();
}

void sendDataNetwork(Mensagem^ mensagem, NetworkStream^ stream) {
	array<Byte>^ bytes = gcnew array<Byte>(BUFFER_SIZE);
	String^ out;

	//Serializar Mensagem
	out = SerializeMensagem(mensagem);

	bytes = BitConverter::GetBytes(out->Length);
	stream->Write(bytes, 0, 4);

	bytes = Text::Encoding::ASCII->GetBytes(out);
	stream->Write(bytes, 0, bytes->Length);
}


void server() {
	/*Console::WriteLine("Digite a porta: ");
	Int32 port = Convert::ToInt32(Console::ReadLine());*/
	Int32 port = 1000;

	IPAddress^ ipAddress;
	for each (auto addr in Dns::GetHostEntry(Dns::GetHostName())->AddressList) {
		if (addr->AddressFamily == AddressFamily::InterNetwork) {
			ipAddress = addr;
			break;
		}
	}
	TcpListener^ server = gcnew TcpListener(ipAddress, port);
	server->Start();

	Partida^ first = gcnew Partida();
	NetworkStream^ TempStreamPlayer;
	array<Byte>^bytes = gcnew array<Byte>(BUFFER_SIZE);


	first->jogadores = gcnew array<NetworkStream^>(2);


	//Aguarda Player 1
	Console::WriteLine("Aguardando conexao de player 1");
	TcpClient^ client = server->AcceptTcpClient();
	Console::WriteLine("Cliente conectado!\n");
	TempStreamPlayer = client->GetStream();
	first->jogadores[0] = TempStreamPlayer;
	////
	
	//Aguarda Player 2
	Console::WriteLine("Aguardando conexao de player 2");
	client = server->AcceptTcpClient();
	TempStreamPlayer = client->GetStream();
	first->jogadores[1] = TempStreamPlayer;
	Console::WriteLine("Cliente conectado!\n");
	////

	array<Byte>^ data;
	StringBuilder^ tempString = gcnew StringBuilder();

	for (;;) {
		try
		{
			XmlSerializer^ xmlSerializer;
			StringWriter^ streamSerializer;
			StringReader^ stringReader;
			Mensagem^ sendMessage;
			Int32 i;

			int turnPlayer;
			for (int k = 0;;k = ++k % 2) {
				TempStreamPlayer = first->jogadores[k];

				xmlSerializer = gcnew XmlSerializer(array<int>::typeid);
				streamSerializer = gcnew StringWriter();
				xmlSerializer->Serialize(streamSerializer, first->tabuleiro);
				////

				//Criando Mensagem
				sendMessage = gcnew Mensagem();
				sendMessage->nomeFuncao = "tabuleiro";
				sendMessage->parametro = streamSerializer->ToString();
				streamSerializer->Close();
				////

				sendDataNetwork(sendMessage, TempStreamPlayer);

				//Thread::Sleep(50);

				////Autorizando jogada

				//Criando Mensagem
				sendMessage = gcnew Mensagem();
				sendMessage->nomeFuncao = "youturn";
				sendMessage->parametro = "youturn";

				sendDataNetwork(sendMessage, TempStreamPlayer);

				imprimirTabuleiro(first->tabuleiro);



				tempString->Clear();
				tempString->Append(receiveDataNetwork(TempStreamPlayer));

				Mensagem^ out = deserializeMensagem(tempString->ToString());


				if (k == 0) {
					first->tabuleiro[Convert::ToInt32(out->parametro) - 1] = -1;
				}
				else {
					first->tabuleiro[Convert::ToInt32(out->parametro) - 1] = 1;
				}
				imprimirTabuleiro(first->tabuleiro);
				/////

				//Serializando tabuleiro
				xmlSerializer = gcnew XmlSerializer(array<int>::typeid);
				streamSerializer = gcnew StringWriter();
				xmlSerializer->Serialize(streamSerializer, first->tabuleiro);

				//Criando Mensagem
				sendMessage = gcnew Mensagem();
				sendMessage->nomeFuncao = "tabuleiro";
				sendMessage->parametro = streamSerializer->ToString();
				streamSerializer->Close();

				//Serializando Mensagem a ser enviada
				xmlSerializer = gcnew XmlSerializer(Mensagem::typeid);
				streamSerializer = gcnew StringWriter();
				xmlSerializer->Serialize(streamSerializer, sendMessage);

				data = Text::Encoding::ASCII->GetBytes(streamSerializer->ToString());
				streamSerializer->Close();
				for (int i = 0; i < first->jogadores->Length; i++) {
					TempStreamPlayer = first->jogadores[i];
					bytes = BitConverter::GetBytes(streamSerializer->ToString()->Length);
					TempStreamPlayer->Write(bytes, 0, 4);

					TempStreamPlayer->Write(data, 0, data->Length);
				}

				if (checarVitoria(first->tabuleiro) != 0) {
					if (k == 0) {
						//Criando Mensagem
						sendMessage = gcnew Mensagem();
						sendMessage->nomeFuncao = "resultado";
						sendMessage->parametro = "Voce ganhou!";

						//Serializando Mensagem a ser enviada
						xmlSerializer = gcnew XmlSerializer(Mensagem::typeid);
						streamSerializer = gcnew StringWriter();
						xmlSerializer->Serialize(streamSerializer, sendMessage);

						data = Text::Encoding::ASCII->GetBytes(streamSerializer->ToString());
						TempStreamPlayer->Write(data, 0, data->Length);
						break;
					}
					else {
						Console::WriteLine("Player 2 venceu");
						break;
					}
				}
			}
		}
		catch (SocketException^ e) {
			Console::WriteLine("Conexão perdida");
			TempStreamPlayer->Close();
		}
		catch (IOException^ e) {
			Console::WriteLine("Cliente desconectado");
			TempStreamPlayer->Close();
			break;
		}

		/*Thread^ newThread = gcnew Thread(gcnew ParameterizedThreadStart(&threadServer));
		array<Object^>^ teste = { player1, player2 };
		newThread->Start(teste);*/
	}
}






void client() {
	/*Console::WriteLine("Digite a porta: ");
	Int32 port = Convert::ToInt32(Console::ReadLine());*/
	Int32 port = 1000;

	TcpClient^ client = gcnew TcpClient("192.168.25.50", port);

	NetworkStream^ stream = client->GetStream();
	
	Int32 i;
	

	StringBuilder^ receiveString = gcnew StringBuilder();
	StringReader^ stringReader;
	StringWriter^ stringWriter;
	XmlSerializer^ serializer;
	Mensagem^ sendMessage;
	Mensagem^ receiveMessage;
	array<int>^ tabuleiro;

	while (true) {
		try {

			//Receber Mensagem
			receiveString->Clear();
			receiveString->Append(receiveDataNetwork(stream));
			////

			//Desseliarizar Mensagem
			receiveMessage = deserializeMensagem(receiveString->ToString());
			////
			
			////Imprimir tabuleiro
			if (receiveMessage->nomeFuncao == "tabuleiro") {
				stringReader = gcnew StringReader(receiveMessage->parametro);
				serializer = gcnew XmlSerializer(array<int>::typeid);
				tabuleiro = dynamic_cast<array<int>^>(serializer->Deserialize(stringReader));
				imprimirTabuleiro(tabuleiro);
				////
			}
			else if (receiveMessage->nomeFuncao == "youturn") {
				////////Fazer jogada
				Console::Write("Digite sua jogada: ");

				//Criando Mensagem
				Mensagem^ sendMessage = gcnew Mensagem();
				sendMessage->nomeFuncao = "jogada";
				sendMessage->parametro = Console::ReadLine();

				sendDataNetwork(sendMessage, stream);
				/////////
			}
		}
		catch (IOException^ e) {
			Console::Write(e->Message);
			break;
		}
	}
}

void spec() {

}

int main(array<System::String ^> ^args)
{
	Console::Write("1 - Servidor\n2 - Jogador\n3 - Espectador\nDigite uma opcao: ");
	String^ input = Console::ReadLine();

	if (input == "1") {
		server();
	}
	else if (input == "2") {
		client();
	}
	else if (input == "3") {
		Int32 port = 1000;
		TcpClient^ client = gcnew TcpClient("192.168.25.50", port);

		NetworkStream^ stream = client->GetStream();
	}
	else {
		Console::WriteLine("Opcao invalida");
	}

	return 0;
}
