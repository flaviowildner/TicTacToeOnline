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
using namespace System::Collections::Generic;


using namespace std;

#define BUFFER_SIZE 512

////PROTOCOLO
#define TABULEIRO "0"
#define SUA_VEZ "1"
#define JOGADA "2"
#define RESULTADO "3"
#define GANHOU "4"
#define PERDEU "5"
#define VELHA "6"
#define JOGAR_NOVAMENTE "7"
#define ID_PLAYER "8"
////


void specServer(Object^ parametros);
void specClient();


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
	for (int i = 0; i<8; i++){
		if (tabuleiro[wins[i][0]] != 0 && tabuleiro[wins[i][0]] == tabuleiro[wins[i][1]] && tabuleiro[wins[i][0]] == tabuleiro[wins[i][2]])
			return tabuleiro[wins[i][0]];
	}
	for (int i = 0, count = 0; i < 9; i++, count++) {
		if (tabuleiro[i] == 0)
			break;
		if (count == 8)
			return 2;
	}
	return 0;
}

String^ SerializeMensagem(Object^ mensagem, Type^ tipo) {
	XmlSerializer^ serializer;
	StringWriter^ stringWriter;

	serializer = gcnew XmlSerializer(tipo);
	stringWriter = gcnew StringWriter();
	serializer->Serialize(stringWriter, mensagem);

	return stringWriter->ToString();
}

Object^ deserializeMensagem(String^ mensagem, Type^ tipo) {
	StringReader^ stringReader;

	stringReader = gcnew StringReader(mensagem);
	XmlSerializer^ serializer = gcnew XmlSerializer(tipo);
	return dynamic_cast<Object^>(serializer->Deserialize(stringReader));
}

String^ receiveDataNetwork(NetworkStream^ TempStreamServer) {
	array<Byte>^ bytes = gcnew array<Byte>(BUFFER_SIZE);
	StringBuilder^ retorno = gcnew StringBuilder();
	Int32 sizeOfNextMessage, i;

	////Recebendo tamanho da próxima Mensagem
	i = TempStreamServer->Read(bytes, 0, 4);
	sizeOfNextMessage = BitConverter::ToInt32(bytes, 0);
	////

	////Recebendo a Mensagem
	do
	{
		if (sizeOfNextMessage > bytes->Length) {
			i = TempStreamServer->Read(bytes, 0, bytes->Length);
		}
		else {
			i = TempStreamServer->Read(bytes, 0, sizeOfNextMessage);
		}
		sizeOfNextMessage -= bytes->Length;
		retorno->Append(Encoding::ASCII->GetString(bytes, 0, i));
	} while (sizeOfNextMessage > 0);
	////

	return retorno->ToString();
}

void sendMessageNetwork(Mensagem^ mensagem, NetworkStream^ TempStreamServer) {
	array<Byte>^ bytes = gcnew array<Byte>(BUFFER_SIZE);
	String^ out;

	//Serializar Mensagem
	out = SerializeMensagem(mensagem, mensagem->GetType());

	bytes = BitConverter::GetBytes(out->Length);
	TempStreamServer->Write(bytes, 0, 4);

	bytes = Text::Encoding::ASCII->GetBytes(out);
	TempStreamServer->Write(bytes, 0, bytes->Length);
}

Mensagem^ criarMensagem(String^ nomeFuncao, String^ parametro) {
	Mensagem^ temp = gcnew Mensagem();
	temp->nomeFuncao = nomeFuncao;
	temp->parametro = parametro;
	return temp;
}


void server() {
	Console::WriteLine("Digite a porta: ");
	Int32 port = Convert::ToInt32(Console::ReadLine());

	TcpListener^ server = gcnew TcpListener(port);
	server->Start();

	Partida^ partida = gcnew Partida();
	NetworkStream^ TempStreamPlayer;
	array<Byte>^ bytes = gcnew array<Byte>(BUFFER_SIZE);

	Mensagem^ tempEnviaMensagem = gcnew Mensagem();
	Mensagem^ tempRecebeMensagem = gcnew Mensagem();

	for (;;) {

		//Aguardando Player 1
		Console::Clear();
		Console::WriteLine("Aguardando conexao de player 1...");
		TcpClient^ client = server->AcceptTcpClient();
		Console::WriteLine("Cliente conectado!\n");
		TempStreamPlayer = client->GetStream();
		partida->jogadores->Add(TempStreamPlayer);

		tempEnviaMensagem = criarMensagem(ID_PLAYER, "1");
		sendMessageNetwork(tempEnviaMensagem, TempStreamPlayer);
		////


		//Aguardando Player 2
		Console::WriteLine("Aguardando conexao de player 2...");
		client = server->AcceptTcpClient();
		TempStreamPlayer = client->GetStream();
		partida->jogadores->Add(TempStreamPlayer);
		Console::WriteLine("Cliente conectado!\n");

		tempEnviaMensagem = criarMensagem(ID_PLAYER, "2");
		sendMessageNetwork(tempEnviaMensagem, TempStreamPlayer);
		////


		//Inicia thread para espectadores
		Thread^ specThread = gcnew Thread(gcnew ParameterizedThreadStart(specServer));
		specThread->Start(gcnew Tuple<TcpListener^, Partida^>(server, partida));
		//

		StringBuilder^ tempString = gcnew StringBuilder();
		try
		{
			for (int k = 0;; k = ++k % 2) {
				TempStreamPlayer = partida->jogadores[k];

				////Gerando tabuleiro
				//Criando Mensagem
				tempEnviaMensagem = criarMensagem(TABULEIRO, SerializeMensagem(partida->tabuleiro, partida->tabuleiro->GetType()));
				//Envia Mensagem
				for (int i = 0; i < partida->jogadores->Count; i++) {
					sendMessageNetwork(tempEnviaMensagem, partida->jogadores[i]);
				}
				for (int i = 0; i < partida->espectadores->Count; i++) {
					sendMessageNetwork(tempEnviaMensagem, partida->espectadores[i]);
				}

				////Autorizando jogada
				//Criando Mensagem
				tempEnviaMensagem = criarMensagem(SUA_VEZ, SUA_VEZ);
				//Envia Mensagem
				sendMessageNetwork(tempEnviaMensagem, TempStreamPlayer);
				//
				////

				imprimirTabuleiro(partida->tabuleiro);

				////Aguardando jogadas
				tempString->Clear();
				tempString->Append(receiveDataNetwork(TempStreamPlayer));
				tempRecebeMensagem = dynamic_cast<Mensagem^>(deserializeMensagem(tempString->ToString(), Mensagem::typeid));
				/////

				////Atualizando tabuleiro
				if (k == 0) {
					partida->tabuleiro[Convert::ToInt32(tempRecebeMensagem->parametro) - 1] = -1;
				}
				else {
					partida->tabuleiro[Convert::ToInt32(tempRecebeMensagem->parametro) - 1] = 1;
				}
				imprimirTabuleiro(partida->tabuleiro);
				/////

				//Serializando tabuleiro
				SerializeMensagem(partida->tabuleiro, partida->tabuleiro->GetType());

				//Criando Mensagem
				tempEnviaMensagem = criarMensagem(TABULEIRO, SerializeMensagem(partida->tabuleiro, partida->tabuleiro->GetType()));

				//Enviando Mensagem
				sendMessageNetwork(tempEnviaMensagem, TempStreamPlayer);

				//Checa se há vitória
				if (checarVitoria(partida->tabuleiro) != 0) {
					tempEnviaMensagem->nomeFuncao = RESULTADO;

					if (checarVitoria(partida->tabuleiro) == 2) {
						tempEnviaMensagem->parametro = VELHA;

						//Envia Mensagem de resultado para player 1
						TempStreamPlayer = partida->jogadores[k];
						sendMessageNetwork(tempEnviaMensagem, TempStreamPlayer);

						//Envia Mensagem de resultado para player 2
						TempStreamPlayer = partida->jogadores[1 - k];
						sendMessageNetwork(tempEnviaMensagem, TempStreamPlayer);

						tempEnviaMensagem->parametro = "Player " + Convert::ToString(k + 1) + " ganhou";
						for (int i = 0; i < partida->espectadores->Count; i++) {
							sendMessageNetwork(tempEnviaMensagem, partida->espectadores[i]);
						}
					}
					else {
						//Envia Mensagem de resultado para player 1
						tempEnviaMensagem->parametro = GANHOU;
						TempStreamPlayer = partida->jogadores[k];
						sendMessageNetwork(tempEnviaMensagem, TempStreamPlayer);


						//Envia Mensagem de resultado para player 2
						TempStreamPlayer = partida->jogadores[1 - k];
						tempEnviaMensagem->parametro = PERDEU;
						sendMessageNetwork(tempEnviaMensagem, TempStreamPlayer);

						//Envia Mensagem de resultado para os espectadores
						tempEnviaMensagem->parametro = "Player " + Convert::ToString(k + 1) + " ganhou";
						for (int i = 0; i < partida->espectadores->Count; i++)
							sendMessageNetwork(tempEnviaMensagem, partida->espectadores[i]);
					}
					
					int n = 0;
					for (int k = 0; k < 2; k++) {
						tempString->Clear();
						tempString->Append(receiveDataNetwork(partida->jogadores[k]));
						tempRecebeMensagem = dynamic_cast<Mensagem^>(deserializeMensagem(tempString->ToString(), Mensagem::typeid));
						
						if (tempRecebeMensagem->nomeFuncao == JOGAR_NOVAMENTE) {
							if (tempRecebeMensagem->parametro == "1") {
								n++;
							}
							else if (tempRecebeMensagem->parametro == "0") {

							}
						}
					}
					if (n != 2) {
						break;
					}
					else {
						for (int i = 0; i < 9; i++) {
							partida->tabuleiro[i] = 0;
						}
					}
				}
			}
		}
		catch (SocketException^ e) {
			Console::WriteLine("Conexão perdida" + e->Message);
			TempStreamPlayer->Close();
		}
		catch (IOException^ e) {
			Console::WriteLine("Cliente desconectado" + e->Message);
			TempStreamPlayer->Close();
			break;
		}
	}
}


void client() {
	Console::WriteLine("Digite o IP: ");
	String^ ipAdress = Console::ReadLine();
	Console::WriteLine("Digite a porta: ");
	Int32 port = Convert::ToInt32(Console::ReadLine());

	TcpClient^ client = gcnew TcpClient(ipAdress, port);

	NetworkStream^ TempStreamServer = client->GetStream();

	StringBuilder^ receiveString = gcnew StringBuilder();
	StringReader^ stringReader;
	XmlSerializer^ serializer;
	Mensagem^ sendMessage;
	Mensagem^ receiveMessage;
	array<int>^ tabuleiro;
	int MyIdPlayer;
	int jogada;
	int op;

	try {
		while (TempStreamServer != nullptr) {
			//Receber Mensagem
			receiveString->Clear();
			receiveString->Append(receiveDataNetwork(TempStreamServer));
			////

			//Desseliarizar Mensagem
			receiveMessage = dynamic_cast<Mensagem^>(deserializeMensagem(receiveString->ToString(), Mensagem::typeid));
			////

			////Imprimir tabuleiro
			if (receiveMessage->nomeFuncao == TABULEIRO) {
				stringReader = gcnew StringReader(receiveMessage->parametro);
				serializer = gcnew XmlSerializer(array<int>::typeid);
				tabuleiro = dynamic_cast<array<int>^>(serializer->Deserialize(stringReader));
				imprimirTabuleiro(tabuleiro);
				////
			}
			else if (receiveMessage->nomeFuncao == SUA_VEZ) {
				////////Fazer jogada
				Console::Write("Sua vez. Digite sua jogada: ");
				do {
					jogada = Convert::ToInt32(Console::ReadLine());
					if (tabuleiro[jogada - 1] == 0) {
						sendMessage = criarMensagem(JOGADA, Convert::ToString(jogada));
					}
					else {
						imprimirTabuleiro(tabuleiro);
						Console::Write("Jogada inválida. Tente outra posição: ");
					}
				} while (tabuleiro[jogada - 1] != 0);
				sendMessageNetwork(sendMessage, TempStreamServer);
				/////////
			}
			else if (receiveMessage->nomeFuncao == RESULTADO) {
				if (receiveMessage->parametro == GANHOU) {
					Console::WriteLine("Você ganhou!\n");
				}
				else if (receiveMessage->parametro == PERDEU) {
					Console::WriteLine("Você perdeu!\n");
				}
				else if (receiveMessage->parametro == VELHA) {
					Console::WriteLine("Velha!\n");
				}

				Console::WriteLine("Jogar novamente?\n1- Sim\n2- Não");
				do {
					op = Convert::ToInt32(Console::ReadLine());
					if (op == 1) {
						sendMessage = criarMensagem(JOGAR_NOVAMENTE, "1");
					}
					else if(op == 2) {
						sendMessageNetwork(sendMessage, TempStreamServer);
						sendMessage = criarMensagem(JOGAR_NOVAMENTE, "0");
						TempStreamServer->Close();
						TempStreamServer = nullptr;
						break;
					}
					else {
						Console::WriteLine("Opção inválida. Tente novamente: ");
						continue;
					}
					sendMessageNetwork(sendMessage, TempStreamServer);
				} while (op < 1 || op > 2);
			}
			else if(receiveMessage->nomeFuncao == ID_PLAYER) {
				MyIdPlayer = Convert::ToInt32(receiveMessage->parametro);
			}
		}
	}
	catch (IOException^ e) {
		Console::Write(e->Message);
	}
}

void specServer(Object^ parametros) {
	NetworkStream^ TempStreamSpec;
	TcpClient^ client;
	Mensagem^ tempEnviaMensagem;

	auto args = safe_cast<Tuple<TcpListener^, Partida^>^>(parametros);

	TcpListener^ server = args->Item1;
	Partida^ partida = args->Item2;

	while (true) {
		client = server->AcceptTcpClient();
		TempStreamSpec = client->GetStream();
		partida->espectadores->Add(TempStreamSpec);
		tempEnviaMensagem = criarMensagem(TABULEIRO, SerializeMensagem(partida->tabuleiro, partida->tabuleiro->GetType()));
		sendMessageNetwork(tempEnviaMensagem, TempStreamSpec);
	}
}


void specClient() {
	Console::WriteLine("Digite o IP: ");
	String^ ipAdress = Console::ReadLine();
	Console::WriteLine("Digite a porta: ");
	Int32 port = Convert::ToInt32(Console::ReadLine());


	TcpClient^ client = gcnew TcpClient(ipAdress, port);

	StringBuilder^ receiveString = gcnew StringBuilder();
	StringReader^ stringReader;
	XmlSerializer^ serializer;
	Mensagem^ receiveMessage;
	array<int>^ tabuleiro;

	NetworkStream^ TempStreamServer = client->GetStream();

	while (TempStreamServer != nullptr) {
		//Receber Mensagem
		receiveString->Clear();
		receiveString->Append(receiveDataNetwork(TempStreamServer));
		////

		//Desseliarizar Mensagem
		receiveMessage = dynamic_cast<Mensagem^>(deserializeMensagem(receiveString->ToString(), Mensagem::typeid));
		////


		////Imprimir tabuleiro
		if (receiveMessage->nomeFuncao == TABULEIRO) {
			stringReader = gcnew StringReader(receiveMessage->parametro);
			serializer = gcnew XmlSerializer(array<int>::typeid);
			tabuleiro = dynamic_cast<array<int>^>(serializer->Deserialize(stringReader));
			imprimirTabuleiro(tabuleiro);
			////
		}
		else if (receiveMessage->nomeFuncao == RESULTADO) {
			if (receiveMessage->parametro == GANHOU) {
				Console::WriteLine("Você ganhou!\n");
			}
			else if (receiveMessage->parametro == PERDEU) {
				Console::WriteLine("Você perdeu\n");
			}
			TempStreamServer->Close();
			TempStreamServer = nullptr;
			break;
		}
	}
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
		specClient();
	}
	else {
		Console::WriteLine("Opcao invalida");
	}

	return 0;
}