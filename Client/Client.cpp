#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <inaddr.h>
#include <stdio.h>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

int recv_int(SOCKET socket, int* num);
void recv_float(SOCKET socket, float* num);
void recv_matrix(SOCKET socket, float* matrix, int len);
void send_vector(SOCKET socket, float* vector, int len);
float get_determinant(float* matrix, int n, bool is_clear_memory);
float* get_algebraic_complement_row(float* matrix, int n, int removedRowIndex, float main_determinant);

const char SERVER_IP[] = "127.0.0.1";
const int PORT = 8000;
const short BUFFER_SIZE = 65536;

int main(void)
{
	in_addr server_in_addr;

	if (inet_pton(AF_INET, SERVER_IP, &server_in_addr) <= 0)
	{
		std::cout << "Error in IP translation to special numeric format" << std::endl;

		return 1;
	}


	// WinSock initialization
	WSADATA wsData;

	if (WSAStartup(MAKEWORD(2, 2), &wsData) != 0)
	{
		std::cout << "Error WinSock version initializaion #" << WSAGetLastError() << std::endl;

		return 1;
	}
	
	std::cout << "WinSock initialization is OK" << std::endl;

	// Socket initialization
	SOCKET client_socket = socket(AF_INET, SOCK_STREAM, 0);

	if (client_socket == INVALID_SOCKET)
	{
		std::cout << "Error initialization socket #" << WSAGetLastError() << std::endl;

		closesocket(client_socket);
		WSACleanup();

		return 1;
	}
	
	std::cout << "Client socket initialization is OK" << std::endl;

	// Establishing a connection to Server
	sockaddr_in server_sockddr_in;

	ZeroMemory(&server_sockddr_in, sizeof(sockaddr_in));

	server_sockddr_in.sin_family = AF_INET;
	server_sockddr_in.sin_addr = server_in_addr;
	server_sockddr_in.sin_port = htons(PORT);

	if (connect(client_socket, (sockaddr*)&server_sockddr_in, sizeof(sockaddr_in)) != 0)
	{
		std::cout << "Connection to Server is FAILED. Error #" << WSAGetLastError() << std::endl;

		closesocket(client_socket);
		WSACleanup();

		return 1;
	}
	
	std::cout << "Connection established SUCCESSFULLY." << std::endl;

	/*float a;

	recv_float(client_socket, &a);

	std::cout << a;
	return 1;*/

	/*float* matrix = new float[16];

	recv_matrix(client_socket, matrix, 16);

	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			std::cout << matrix[i * 4 + j] << ' ';
		}

		std::cout << std::endl;
	}

	return 1;*/

	while (true)
	{
		int n;

		if (recv_int(client_socket, &n) == INVALID_SOCKET)
		{
			break;
		}

		std::cout << "n: " << n << std::endl;

		float* matrix = new float[n * n];

		recv_matrix(client_socket, matrix, n * n);

		for (int i = 0; i < n; ++i)
		{
			for (int j = 0; j < n; ++j)
			{
				std::cout << matrix[i * n + j] << ' ';
			}

			std::cout << std::endl;
		}

		float determinant;

		recv_float(client_socket, &determinant);

		std::cout << "Determinant: " << determinant << std::endl;

		int removedRowIndex;

		recv_int(client_socket, &removedRowIndex);

		std::cout << "Removed row index: " << removedRowIndex << std::endl;

		float* row = get_algebraic_complement_row(matrix, n, removedRowIndex, determinant);

		send_vector(client_socket, row, n);
	}

	closesocket(client_socket);
	WSACleanup();

	return 0;
}

int recv_int(SOCKET socket, int* num)
{
	return recv(socket, (char*)num, sizeof(int), 0);
}

void recv_float(SOCKET socket, float* num)
{
	recv(socket, (char*)num, sizeof(float), 0);
}

void recv_matrix(SOCKET socket, float* matrix, int len)
{
	recv(socket, (char*)matrix, len * sizeof(float), 0);
}

void send_vector(SOCKET socket, float* vector, int len)
{
	send(socket, (char*)vector, len * sizeof(float), 0);
}

float* get_algebraic_complement_row(float* matrix, int n, int removedRowIndex, float main_determinant)
{
	float* row = new float[n];

	std::cout << "Matrix:" << std::endl;
	for (int i = 0; i < n; ++i)
	{
		for (int j = 0; j < n; ++j)
		{
			std::cout << matrix[i * n + j] << ' ';
		}

		std::cout << std::endl;
	}

	for (int removedColumnIndex = 0; removedColumnIndex < n; removedColumnIndex++)
	{
		float* smallerOrderMatrix = new float[(n - 1) * (n - 1)];

		int smallerOrderMatrixRowIndex = 0;
		int smallerOrderMatrixColumnIndex = 0;

		for (int rowIndex = 0; rowIndex < n; rowIndex++)
		{
			if (rowIndex == removedRowIndex)
			{
				continue;
			}

			for (int columnIndex = 0; columnIndex < n; columnIndex++)
			{
				if (columnIndex == removedColumnIndex)
				{
					continue;
				}

				smallerOrderMatrix[(n - 1) * smallerOrderMatrixRowIndex + smallerOrderMatrixColumnIndex] = matrix[n * rowIndex + columnIndex];

				if (smallerOrderMatrixColumnIndex == n - 2)
				{
					smallerOrderMatrixRowIndex++;
					smallerOrderMatrixColumnIndex = 0;
				}
				else
				{
					smallerOrderMatrixColumnIndex++;
				}
			}
		}

		int multiplier = ((removedRowIndex + removedColumnIndex) % 2 == 0) ? 1 : -1;

		std::cout << "Smaller matrix:" << std::endl;
		for (int i = 0; i < (n - 1); ++i)
		{
			for (int j = 0; j < (n - 1); ++j)
			{
				std::cout << smallerOrderMatrix[i * (n - 1) + j] << ' ';
			}

			std::cout << std::endl;
		}

		float determinant = get_determinant(smallerOrderMatrix, (n - 1), false);

		row[removedColumnIndex] = determinant * multiplier / main_determinant;
	}

	return row;
}

float get_determinant(float* matrix, int n, bool is_clear_memory)
{
	if (n == 2)
	{
		return matrix[0] * matrix[3] - matrix[1] * matrix[2];
	}

	for (int rowIndex = n - 2; rowIndex >= 0; rowIndex--)
	{
		float k = (-1) * matrix[n * (rowIndex + 1)] / matrix[n * rowIndex];

		for (int columnIndex = 0; columnIndex < n; columnIndex++)
		{
			matrix[n * (rowIndex + 1) + columnIndex] += matrix[n * rowIndex + columnIndex] * k;
		}
	}

	float* newMatrix = new float[(n - 1) * (n - 1)];

	for (int rowIndex = 1; rowIndex < n; rowIndex++)
	{
		for (int columnIndex = 1; columnIndex < n; columnIndex++)
		{
			newMatrix[(n - 1) * (rowIndex - 1) + columnIndex - 1] = matrix[n * rowIndex + columnIndex];
		}
	}

	float value = matrix[0];

	if (is_clear_memory)
	{
		delete[] matrix;
	}

	return value * get_determinant(newMatrix, n - 1, true);
}