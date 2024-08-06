// Server.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "framework.h"
#include "Server.h"
#include "afxsock.h"
#include <filesystem>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// The one and only application object

CWinApp theApp;

using namespace std;
struct Datafile {
	char filename[20];
	char size[10];
};
//Function to turn ON/OFF cursor in console
void ShowCur(bool CursorVisibility)
{
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO ConCurInf;

	ConCurInf.dwSize = 10;
	ConCurInf.bVisible = CursorVisibility;	

	SetConsoleCursorInfo(handle, &ConCurInf);
}
//Get info all file Server can upload to clients
void GetFileData(Datafile files[], int& n) {
	ifstream fin("filedata.txt", ios::in);
	if (!fin) cout << "file had't been existed!!   \n";
	else {
		int i = 0;
		string Filename, Size;
		while (!fin.eof()) {
			getline(fin, Filename, ' ');
			strcpy_s(files[i].filename, Filename.c_str());
			getline(fin, Size, '\n');
			strcpy_s(files[i].size, Size.c_str());
			i++;
		}
		n = i;
	}
}
bool CheckExist(Datafile files[], char require_file[], int n) {
	for (int i = 0; i < n; i++)
		if (strcmp(require_file, files[i].filename) == 0)
			return true;
	return false;
}
//Send buffer to clients
bool Send_buffer(CSocket& Server, char* buffer, int buffer_size) {
	int total_size_send = 0;
	while (total_size_send < buffer_size) {
		int bytes_send = 0;
		bytes_send = Server.Send(&buffer[total_size_send], buffer_size - total_size_send);
		if (bytes_send <= 0) return false;
		total_size_send += bytes_send;
	}	
	return true;
}
//Receive buffer from clients
bool Receive_buffer(CSocket& Server, char* buffer, int buffer_size) {
	int total_size_recv = 0;
	while (total_size_recv < buffer_size) {
		int bytes_recv = 0;
		bytes_recv = Server.Receive(&buffer[total_size_recv], buffer_size - total_size_recv);
		if (bytes_recv <= 0) return false;
		total_size_recv += bytes_recv;
	}
	return true;
}
//Send file to clients
bool Send_file(CSocket& Server, char require_file[]) {
	string filename(require_file);
	ifstream fin("Data/" + filename, ios::in | ios::binary);
	if (!fin) { cout << "Cannot open " << filename << " !!\n"; return false; }
	int bytes_send = 0, total_bytes_send = 0;
	//Get file size
	fin.seekg(0, ios::end);
	int file_size = fin.tellg();
	fin.seekg(0, ios::beg);
	//Create size buffer to store file data
	const int BUFFER_SIZE = 10000;
	char buffer[BUFFER_SIZE];
	bytes_send = Server.Send((char*)&file_size, sizeof(file_size), 0);
	if (bytes_send < 1) { cout << "Cannot send file!!\n"; return false; }
	cout << ".......Uploading " << filename;
	while (total_bytes_send < file_size) {
		bytes_send = min(file_size - total_bytes_send, BUFFER_SIZE);
		fin.read((char*)&buffer, bytes_send);
		if (!Send_buffer(Server, buffer, bytes_send)) return false;
		total_bytes_send += bytes_send;
		cout << "\r" << int((total_bytes_send / float(file_size)) * 100) << "%";
	}
	cout << endl;
	fin.close();
	return true;
}
////Upload file to clients
//void SendFile(CSocket &Client, char require_file[]) {
//	string filename(require_file);
//	fstream fin("Data/" + filename, ios::in | ios::binary);
//	if (!fin) cout << "Cannot open file!!\n";
//	else {
//		int file_size, bytes_send = 0, total_data = 0;
//		//Get file size
//		fin.seekg(0, ios::end);
//		file_size = fin.tellg();
//		//file_size = std::filesystem::file_size(filename);
//		fin.seekg(0, ios::beg);
//		//Create size buffer to store file data
//		char buffer1[1024];
//		//Send file size to client
//		bytes_send = Client.Send((char*)&file_size, sizeof(file_size), 0);
//		if (bytes_send < sizeof(file_size)) { cout << "Cannot send file!!\n"; return; }
//		cout << ".......Uploading " << filename;
//		while (total_data < file_size) {
//			//Using char[] when it has still has more than 1028 bytes to upload fastly
//			if (total_data <= file_size - 1024) {
//				//Get file data
//				fin.read((char*)&buffer1, sizeof(buffer1));
//				//Send file data to client
//				bytes_send = Client.Send((char*)&buffer1, sizeof(buffer1), 0);
//				if (bytes_send < 1) { cout << "\nCannot send file!!\n"; return; }
//				total_data += bytes_send;
//				cout << "\r" << int((total_data / float(file_size)) * 100) << "%";
//			}
//			//Using char* when it just has a little bit of bytes to upload exactly
//			else {
//				char* buffer2;
//				buffer2 = new char[file_size - total_data];
//				//Get file data
//				fin.read(buffer2, file_size - total_data);
//				//Send file data to client
//				bytes_send = Client.Send(buffer2, file_size - total_data, 0);
//				if (bytes_send < 1) { cout << "\nCannot send file!!\n"; return; }
//				total_data += bytes_send;
//				cout << "\r" << int((total_data / float(file_size)) * 100) << "%";
//				delete[] buffer2;
//			}
// 		}
//		cout << endl;
//	}
//	fin.close();
//}
int main()
{
	ShowCur(0);
    int nRetCode = 0;

    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // initialize MFC and print and error on failure
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: code your application's behavior here.
            wprintf(L"Fatal Error: MFC initialization failed\n");
            nRetCode = 1;
        }
        else
        {
			CSocket Server;
			unsigned int port = 1234;
			AfxSocketInit(NULL);
			//Create Server
			if (!Server.Create(port,SOCK_STREAM, NULL)) {
				cout << "Cannot create server!!\n";
				return nRetCode;
			}
			//Listen to Clients
			if (!Server.Listen(5)) {
				cout << "Cannot listen...\n";
				return nRetCode;
			}
			//Number of Clients, Server can transfer file
			int num_clients;
			cout << "Input num of clients: "; cin >> num_clients;
			CSocket* SocketClients = new CSocket[num_clients];

			for (int i = 0; i < num_clients; i++) {
				cout << "\rWaiting for connection...";
				//Accept and send the number of being clients to clients
				Server.Accept(SocketClients[i]);
				cout << "\r" << "Connect to Client " << i + 1 << " successfully!!\n";
				//Send Clients list of file can be downloaded
				SocketClients[i].Send((char*)&i, sizeof(int), 0);
				Datafile files[10]; int n_list = 0;
				GetFileData(files, n_list); int files_data = 0;
				SocketClients[i].Send((char*)&n_list, sizeof(n_list), 0);
				files_data = SocketClients[i].Send(files, sizeof(files), 0);
				if (files_data == 0) { cout << "Cannot send files data!!\n"; continue; }
				//Using idx as the number of file has been transfered
				//So, Server had't to transfer them again
				int idx = 0; bool flag;
				//Real time transfering
				while (SocketClients[i].Receive((char*)&flag, sizeof(flag), 0) > 0) {
					char require_file[20][20]; int list_refile; int data_file = 0;
					//Receive list file need to be download
					SocketClients[i].Receive((char*)&list_refile, sizeof(list_refile), 0);
					data_file = SocketClients[i].Receive((char*)&require_file, sizeof(require_file), 0);
					if (data_file == 0) { cout << "Cannot send file!!\n"; continue; }
					SocketClients[i].Receive((char*)&idx, sizeof(idx), 0);
					//Transfer file
					for (; idx < list_refile; idx++) {
						if (CheckExist(files, require_file[idx], n_list)) {
							if (!Send_file(SocketClients[i], require_file[idx])) idx = list_refile;
						}
						else cout << "'" << require_file[idx] << "'" << " is not found!!\n";
					}
					cout << "Waiting for downloading...";
					Sleep(1000); cout << "\r";
				}
				SocketClients[i].Close();
				cout << "Disconnect to Client " << i + 1 << " !!         \n";
			}
			getchar();
			delete [] SocketClients;
			Server.Close();
        }
    }
    else
    {
        // TODO: change error code to suit your needs
        wprintf(L"Fatal Error: GetModuleHandle failed\n");
        nRetCode = 1;
    }
	getchar();
    return nRetCode;
}
