#include <iostream>
#include <vector>
#include <string>
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

constexpr int PORT = 25565;

enum Player {
	SERVER,
	CLIENT
};
using Board = std::vector<std::vector<char>>;

void clearScreen() {
	system("cls");
}

void printBoard(const Board& board) {
	clearScreen();
	for(const auto& row : board) {
		for(const auto& cell : row) {
			std::cout << (cell == 0 ? ' ' : cell) << ' ';
		}
		std::cout << std::endl;
	}
}

bool isValidMove(int col, int row, const Board& board) {
	return col >= 0 && col < 3 && row >= 0 && row < 3 && board[row][col] == 0;
}

char getPlayerChar(Player player) {
	return player == SERVER ? '0' : 'X';
}

void makeMove(Player player, Board& board, SOCKET socket) {
	int row, column;
	do {
		if(player == SERVER) {
			std::cout << "Столбец Строка(0-2): ";
			std::cin >> column >> row;
			send(socket, reinterpret_cast<char*>(&column), sizeof(column), 0);
			send(socket, reinterpret_cast<char*>(&row), sizeof(row), 0);
		}
		else {
			recv(socket, reinterpret_cast<char*>(&column), sizeof(column), 0);
			recv(socket, reinterpret_cast<char*>(&row), sizeof(row), 0);
		}
	} while(!isValidMove(column, row, board));
	board[row][column] = getPlayerChar(player);
}

bool checkDiagonal(char character, const Board& board) {
	bool diag1 = true, diag2 = true;
	for(int i = 0; i < 3; ++i) {
		diag1 &= (board[i][i] == character);
		diag2 &= (board[i][2 - i] == character);
	}
	return diag1 || diag2;
}

bool checkLines(char character, const Board& board) {
	for(int i = 0; i < 3; ++i) {
		bool row = true, col = true;
		for(int j = 0; j < 3; ++j) {
			row &= (board[i][j] == character);
			col &= (board[j][i] == character);
		}
		if(row || col) return true;
	}
	return false;
}

bool isWin(Player player, const Board& board) {
	char character = getPlayerChar(player);
	return checkDiagonal(character, board) || checkLines(character, board);
}

void playGame(Player startingPlayer, SOCKET socket) {
	Board board(3, std::vector<char>(3, 0));
	Player currentPlayer = startingPlayer;
	bool gameOver = false;
	printBoard(board);
	while(!gameOver) {
		makeMove(currentPlayer, board, socket);
		printBoard(board);
		if(isWin(currentPlayer, board)) {
			gameOver = true;
			std::cout << "Игрок " << getPlayerChar(currentPlayer) << " победил!" << std::endl;
		}
		currentPlayer = currentPlayer == SERVER ? CLIENT : SERVER;
	}
}

void runServer() {
	SOCKET serverSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(serverSocket == INVALID_SOCKET) {
		std::cerr << "Socket creation failed with error: " << WSAGetLastError() << std::endl;
		WSACleanup();
		return;
	}

	sockaddr_in service{};
	service.sin_family = PF_INET;
	service.sin_addr.s_addr = INADDR_ANY;
	service.sin_port = htons(PORT);

	if(bind(serverSocket, reinterpret_cast<SOCKADDR*>(&service), sizeof(service)) == SOCKET_ERROR) {
		std::cerr << "Bind failed with error: " << WSAGetLastError() << std::endl;
		closesocket(serverSocket);
		WSACleanup();
		return;
	}

	if(listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
		std::cerr << "Listen failed with error: " << WSAGetLastError() << std::endl;
		closesocket(serverSocket);
		WSACleanup();
		return;
	}

	std::cout << "Жду подключения клиента..." << std::endl;

	SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
	if(clientSocket == INVALID_SOCKET) {
		std::cerr << "Accept failed with error: " << WSAGetLastError() << std::endl;
		closesocket(serverSocket);
		WSACleanup();
		return;
	}

	playGame(SERVER, clientSocket);

	closesocket(clientSocket);
	closesocket(serverSocket);
}

void runClient() {
	std::string ipAddress;
	std::cout << "Введите IP: ";
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	std::getline(std::cin, ipAddress);
	if(ipAddress.empty()) {
		ipAddress = "127.0.0.1";
	}

	SOCKET connectSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(connectSocket == INVALID_SOCKET) {
		std::cerr << "Socket creation failed." << std::endl;
		return;
	}

	sockaddr_in serverAddr{};
	serverAddr.sin_family = PF_INET;
	inet_pton(PF_INET, ipAddress.c_str(), &serverAddr.sin_addr);
	serverAddr.sin_port = htons(PORT);

	if(connect(connectSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
		std::cerr << "Connection failed." << std::endl;
		closesocket(connectSocket);
		return;
	}

	playGame(CLIENT, connectSocket);

	closesocket(connectSocket);
}

int initialize() {
	WSADATA wsaData;
	if(WSAStartup(MAKEWORD(2, 2), &wsaData) != NO_ERROR) {
		std::cerr << "WSAStartup failed with error: " << WSAGetLastError() << std::endl;
		return 1;
	}
	return 0;
}

int main() {
	if(initialize() != 0) {
		return -1;
	}

	SetConsoleCP(1251);
	SetConsoleOutputCP(1251);

	int role;
	std::cout << "Кто ты?\n0) Сервер\n1) Клиент\n";
	std::cin >> role;
	switch(role) {
	case SERVER:
		runServer();
		break;
	case CLIENT:
		runClient();
		break;
	default:
		std::cerr << "Неверный ввод." << std::endl;
		break;
	}

	WSACleanup();
	system("pause");
	return 0;
}
