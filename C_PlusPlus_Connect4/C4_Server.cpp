#include <iostream>
#include <Windows.h>

#include <stdio.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

int GameClients = 0;
string GameTurn = "1";
string GameBoard[6][7];
SOCKET GameSockets[2];
SOCKET ServerSocket;

void SetupGame() {
	for (int x = 0; x < 6; x++) {
		for (int y = 0; y < 7; y++) {
			GameBoard[x][y] = "-";
		}
	}
}

bool CheckWin(string team) {
	for (int y = 0; y < 6; y++) {
		for (int x = 0; x < 4; x++) {
			if (GameBoard[y][x] == team && GameBoard[y][x + 1] == team) {
				if (GameBoard[y][x + 2] == team && GameBoard[y][x + 3] == team) {
					return true;
				}
			}
		}
	}

	for (int y = 0; y < 3; y++) {
		for (int x = 0; x < 7; x++) {
			if (GameBoard[y][x] == team && GameBoard[y + 1][x] == team) {
				if (GameBoard[y + 2][x] == team && GameBoard[y + 3][x] == team) {
					return true;
				}
			}
		}
	}

	for (int y = 0; y < 3; y++) {
		for (int x = 0; x < 7; x++) {
			if (GameBoard[y][x] == team) {
				try {
					if (GameBoard[y + 1][x - 1] == team) {
						if (GameBoard[y + 2][x - 2] == team) {
							if (GameBoard[y + 3][x - 3] == team) {
								return true;
							}
						}
					}
				}
				catch (exception) {}
				try {
					if (GameBoard[y + 1][x + 1] == team) {
						if (GameBoard[y + 2][x + 2] == team) {
							if (GameBoard[y + 3][x + 3] == team) {
								return true;
							}
						}
					}
				}
				catch (exception) {}
			}
		}
	} return false;
}

void PlaceCounter(int column, string team) {
	if (GameBoard[5][column] == "-") {
		GameBoard[5][column] = team;
		return;
	}
	for (int x = 0; x < 6; x++) {
		if (GameBoard[x][column] != "-") {
			GameBoard[x - 1][column] = team;
			break;
		}
	}
}

void Send(string message) {
	for (SOCKET client : GameSockets) {
		send(client, message.c_str(), message.size() + 1, 0);
	} Sleep(750);
}


// GameSend(string type, INT index = NULL, string key = NULL, string value = NULL) would probably be easier to manage.
void GameSend(string type, INT index = NULL, string information[] = NULL) {
	string message = "{'data-type': '" + type + "'";
	if (information != NULL) {
		message += ", '" + information[0] + "': '" + information[1] + "'";
	}
	message += "}";
	if (index == NULL) {
		for (SOCKET client : GameSockets)
			send(client, message.c_str(), message.size() + 1, 0);
	} else {
		send(GameSockets[index], message.c_str(), message.size() + 1, 0);
	} Sleep(750);
}


void ListenClients() {
	while (true) {
		for (int i = 0; i < GameClients; i++) {
			char buf[4096];
			ZeroMemory(buf, 4096);
			int bytes = recv(GameSockets[i], buf, 4096, 0);
			if (bytes <= 0) {
				closesocket(GameSockets[i]);
			} else {
				string receivedData = string(buf);
				if (receivedData.find("column") != string::npos) {  // replace with JSON library
					char column = (int)receivedData[35];
					int columnNumber = column - '0';
					if (GameBoard[0][columnNumber] != "-" || columnNumber < 0 || columnNumber > 6 || (char)receivedData[36] != '\'') {
						GameSend("turn", NULL, new string[2]{ "team", GameTurn });
						i = 1 - i;
						continue;
					}
					Send(receivedData);
					PlaceCounter(columnNumber, GameTurn);
					if (CheckWin(GameTurn)) {
						GameSend("game-win", NULL, new string[2]{ "team", GameTurn });
						Sleep(3000);
						GameSend("kick", NULL, new string[2]{ "message", "Kicked from server (server shutting down)" });
						break;
					}
					if (GameTurn == "1") { GameTurn = "2"; }
					else { GameTurn = "1"; }
					GameSend("turn", NULL, new string[2]{ "team", GameTurn });
				}
			}
		}
	}
}

void SearchClients() {
	ServerSocket = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(5000);
	hint.sin_addr.S_un.S_addr = INADDR_ANY;
	bind(ServerSocket, (sockaddr*)&hint, sizeof(hint));
	listen(ServerSocket, SOMAXCONN);
	while (GameClients < 2) {
		SOCKET clientSocket = accept(ServerSocket, nullptr, nullptr);
		GameSockets[GameClients] = clientSocket;
		GameClients++;
		if (GameClients == 1) {
			GameSend("waiting", 0);
			GameSend("team", 0, new string[2]{ "team", "1" });
		} else {
			GameSend("start");
			GameSend("team", 1, new string[2]{ "team", "2" });
			GameSend("turn", NULL, new string[2]{ "team", "1" });
		}
	} ListenClients();
}

int main() {
	WSACleanup();
	WSADATA wsData;
	WORD ver = MAKEWORD(2, 2);

	int winsockStatus = WSAStartup(ver, &wsData);
	if (winsockStatus != 0) {
		cerr << "Winsock initialisation error!" << endl;
		return winsockStatus;
	}

	SetupGame();
	SearchClients();
	return 0;
}