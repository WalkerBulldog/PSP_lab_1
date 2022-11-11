#include <iostream>
#include <iomanip>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <ctime>

#pragma comment(lib, "Ws2_32.lib")

struct ThreadData
{
	int thread_id;
	int n;
	int client_count;
	SOCKET client_socket;
	float* inversed_matrix;
	float* transposed_matrix;
	float determinant;
};

struct ClientData
{
	SOCKET socket;
	sockaddr_in sockaddr_in;
};

void structures_initialization();
void send_matrix(SOCKET socket, float* matrix, int len);
void send_int(SOCKET socket, int* num);
void send_float(SOCKET socket, float* num);
float get_determinant(float* matrix, int n, bool is_clear_memory);
float* get_transposed_matrix(float* matrix, int n);
DWORD WINAPI thread_func(LPVOID param);
void recv_vector(SOCKET socket, float* vector, int len);
float* multiply(float* matrix, float* vector, int n);
float* get_matrix_copy(float* matrix, int n);

const char SERVER_IP[] = "127.0.0.1";
const int PORT = 8000;
const short BUFFER_SIZE = 65536;

float matrix_A[16] =
{
	 2, 3, 3, 1,
	 3, 5, 3, 2,
	 5, 7, 6, 2,
	 4, 4, 3, 1,
};

float vector_B[4] = { 1, 2, 3, 4 };

//float main_determinant;

const int client_count = 2;

SOCKET client_sockets[client_count];
sockaddr_in clients_sockaddr_in[client_count];

HANDLE handles[client_count];

// temp var
char client_ip[INET_ADDRSTRLEN];

int main(void)
{
	structures_initialization();

	in_addr server_in_addr;

	if (inet_pton(AF_INET, SERVER_IP, &server_in_addr) <= 0)
	{
		std::cout << "Error in IP translation to special numeric format" << std::endl;

		return 1;
	}

	// WinSock initialization
	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		std::cout << "Error WinSock version initializaion #" << WSAGetLastError() << std::endl;

		return 1;
	}

	std::cout << "WinSock initialization is OK" << std::endl;

	// Server socket initialization
	SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);

	if (server_socket == INVALID_SOCKET)
	{
		std::cout << "Error initialization socket #" << WSAGetLastError() << std::endl;

		closesocket(server_socket);
		WSACleanup();

		return 1;
	}

	std::cout << "Server socket initialization is OK" << std::endl;

	// Server socket binding
	sockaddr_in server_sockaddr_in;

	// Initializing servInfo structure
	ZeroMemory(&server_sockaddr_in, sizeof(server_sockaddr_in));

	server_sockaddr_in.sin_family = AF_INET;
	server_sockaddr_in.sin_addr = server_in_addr;
	server_sockaddr_in.sin_port = htons(PORT);

	if (bind(server_socket, (sockaddr*)&server_sockaddr_in, sizeof(server_sockaddr_in)) != 0)
	{
		std::cout << "Error Socket binding to server info. Error #" << WSAGetLastError() << std::endl;

		closesocket(server_socket);
		WSACleanup();

		return 1;
	}

	std::cout << "Binding socket to Server info is OK" << std::endl;

	//Starting to listen to any Clients

	if (listen(server_socket, SOMAXCONN) != 0)
	{
		std::cout << "Can't start to listen to. Error #" << WSAGetLastError() << std::endl;

		closesocket(server_socket);
		WSACleanup();

		return 1;
	}

	std::cout << "Listening..." << std::endl;

	int sockaddr_in_len = sizeof(sockaddr_in);

	while (true)
	{
		std::cout << "Enter client count: ";

		std::string input;

		std::cin >> input;

		int client_count = std::stof(input);

		input.clear();

		ClientData* clients_data = new ClientData[client_count];

		for (int i = 0; i < client_count; ++i)
		{
			int _ = sizeof(sockaddr_in);

			clients_data[i].socket = accept(server_socket, (sockaddr*)&clients_data[i].sockaddr_in, &sockaddr_in_len);

			std::cout << "Client #" << i + 1 << " connected." << std::endl;
		}

		std::cout << "Enter matrix dimension: ";

		std::cin >> input;

		int n = std::stoi(input);

		std::cout << "Generate matrix..." << std::endl << std::endl;

		srand(time(0));

		float* matrix = new float[n * n];

		for (int i = 0; i < n; ++i)
		{
			for (int j = 0; j < n; ++j)
			{
				matrix[i * n + j] = rand() % 40 - 20;

				std::cout << std::setw(4) << matrix[i * n + j];
			}

			std::cout << std::endl;
		}

		std::cout << std::endl;

		float determinant = get_determinant(get_matrix_copy(matrix, n), n, true);

		std::cout << "Determinant: " << determinant << std::endl << std::endl;

		if (determinant == 0)
		{
			for (int i = 0; i < client_count; ++i)
			{
				closesocket(clients_data[i].socket);
			}

			std::cout << "Determinant can not be equals zero!" << std::endl << std::endl << std::endl << std::endl << std::endl;

			continue;
		}

		std::cout << "Generate free members column..." << std::endl << std::endl;

		float* vectorB = new float[n];

		for (int i = 0; i < n; ++i)
		{
			vectorB[i] = rand() % 40 - 20;

			std::cout << vectorB[i] << std::endl;
		}

		std::cout << std::endl;

		float* transposed_matrix = get_transposed_matrix(matrix, n);

		std::cout << "Transposed matrix:" << std::endl;

		for (int i = 0; i < n; ++i)
		{
			for (int j = 0; j < n; ++j)
			{
				std::cout << std::setw(4) << transposed_matrix[i * n + j];
			}

			std::cout << std::endl;
		}

		std::cout << std::endl;

		float* inversed_matrix = new float[n * n];

		int thread_count = std::min<int>(n, client_count);

		HANDLE* handles = new HANDLE[thread_count];

		ThreadData* threads_data = new ThreadData[thread_count];

		for (int i = 0; i < thread_count; ++i)
		{
			threads_data[i].thread_id = i;
			threads_data[i].n = n;
			threads_data[i].determinant = determinant;
			threads_data[i].transposed_matrix = transposed_matrix;
			threads_data[i].inversed_matrix = inversed_matrix;
			threads_data[i].client_count = client_count;
			threads_data[i].client_socket = clients_data[i].socket;

			handles[i] = CreateThread(nullptr, 65536, thread_func, (LPVOID)&threads_data[i], 0, 0);
		}

		WaitForMultipleObjects(thread_count, handles, true, INFINITE);

		std::cout << std::endl;

		std::cout << "Inversed matrix:" << std::endl;

		for (int i = 0; i < n; ++i)
		{
			for (int j = 0; j < n; ++j)
			{
				std::cout << std::setw(12) << inversed_matrix[i * n + j];
			}

			std::cout << std::endl;
		}

		std::cout << std::endl;

		std::cout << "Vector X:" << std::endl;

		float* vectorX = new float[n];

		for (int i = 0; i < n; ++i)
		{
			float value = 0;

			for (int j = 0; j < n; ++j)
			{
				value += inversed_matrix[i * n + j] * vectorB[j];
			}

			vectorX[i] = value;

			std::cout << value << std::endl;
		}

		std::cout << std::endl;

		std::cout << "Finish..." << std::endl << std::endl << std::endl << std::endl << std::endl;

		for (int i = 0; i < client_count; ++i)
		{
			closesocket(clients_data[i].socket);
		}
	}

	closesocket(server_socket);
	WSACleanup();

	return 0;
}

void structures_initialization()
{
	for (int i = 0; i < client_count; ++i)
	{
		ZeroMemory(&clients_sockaddr_in[i], sizeof(sockaddr_in));
	}
}

void send_matrix(SOCKET socket, float* matrix, int len)
{
	send(socket, (char*)matrix, len * sizeof(float), 0);
}

void send_int(SOCKET socket, int* num)
{
	send(socket, (char*)num, sizeof(int), 0);
}

void send_float(SOCKET socket, float* num)
{
	send(socket, (char*)num, sizeof(float), 0);
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

float* get_transposed_matrix(float* matrix, int n)
{
	float* transposedMatrix = new float[n * n];

	for (int rowIndex = 0; rowIndex < n; rowIndex++)
	{
		for (int columnIndex = 0; columnIndex < n; columnIndex++)
		{
			transposedMatrix[n * columnIndex + rowIndex] = matrix[n * rowIndex + columnIndex];
		}
	}

	return transposedMatrix;
}

DWORD WINAPI thread_func(LPVOID param)
{
	ThreadData* thread_data = (ThreadData*)param;

	for (int row_index = thread_data->thread_id; row_index < thread_data->n; row_index += thread_data->client_count)
	{
		send_int(thread_data->client_socket, &thread_data->n);

		send_matrix(thread_data->client_socket, thread_data->transposed_matrix, thread_data->n * thread_data->n);

		send_float(thread_data->client_socket, &thread_data->determinant);

		send_int(thread_data->client_socket, &row_index);

		float* row = new float[thread_data->n];

		recv_vector(thread_data->client_socket, row, thread_data->n);

		for (int i = 0; i < thread_data->n; ++i)
		{
			thread_data->inversed_matrix[row_index * thread_data->n + i] = row[i];
		}
	}

	std::cout << "Client #" << thread_data->thread_id << " performed calculations." << std::endl;

	return 0;
}

void recv_vector(SOCKET socket, float* vector, int len)
{
	recv(socket, (char*)vector, len * sizeof(float), 0);
}

float* multiply(float* matrix, float* vector, int n)
{
	float* resultMatrix = new float[4];

	for (int i = 0; i < 4; ++i)
	{
		float value = 0;

		std::cout << "Row " << i + 1 << ':' << std::endl;

		for (int j = 0; j < 4; ++j)
		{
			std::cout << matrix[4 * i + j] << " + " << vector[j] << ((j == 3) ? "" : " + ");
			value += matrix[4 * i + j] * vector[j];
		}

		resultMatrix[i] = value;
	}

	return resultMatrix;
}

float* get_matrix_copy(float* matrix, int n)
{
	float* matrix_copy = new float[n * n];

	for (int i = 0; i < n; ++i)
	{
		for (int j = 0; j < n; ++j)
		{
			matrix_copy[i * n + j] = matrix[i * n + j];
		}
	}

	return matrix_copy;
}