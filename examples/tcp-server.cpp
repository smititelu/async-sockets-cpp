#include <tcpserver.h>
#include <iostream>

using namespace std;

vector<TCPSocket*> clients;

int main() {
	// Initialize server socket..
	TCPServer tcpServer;

	// When a new client connected:
	tcpServer.onNewConnection = [&](TCPSocket *newClient) {
		clients.push_back(newClient);

		newClient->onSocketClosed = [newClient]() {
			int position = 0;
			for (vector<TCPSocket*>::iterator it = clients.begin(); it != clients.end(); ++it) {
				if (newClient == *it) {
					break;
				}
				position ++;
			}
			clients.erase(clients.begin() + position);
		};

		newClient->onMessageReceived = [newClient](string message) {
			cout << message << endl;
		};
	};

	// Bind the server to a port.
	tcpServer.Bind(80, [](int errorCode, string errorMessage) {
		// BINDING FAILED:
		cout << errorCode << " : " << errorMessage << endl;
	});

	// Start Listening the server.
	tcpServer.Listen([](int errorCode, string errorMessage) {
		// LISTENING FAILED:
		cout << errorCode << " : " << errorMessage << endl;
	});

	string input;
	getline(cin, input);
	while (input != "exit") {
		// commands local to server
		if (input.compare("help") == 0) {
			cout << "Server local commands:" << endl;
			cout << "  help        - prints this" << endl;
			cout << "  count       - prints nr of connected clients" << endl;
			cout << "  list        - prints the connected clients" << endl;
			cout << "Client commands: strat with \"do\"" << endl;
			cout << "  do die      - kill all the clients" << endl;
			cout << "  do info     - info from all the clients" << endl;
		} else if (input.compare("count") == 0) {
			cout << clients.size() << endl;
		} else if (input.compare("list") == 0) {
			for (vector<TCPSocket*>::iterator it = clients.begin(); it != clients.end(); ++it) {
				cout << (*it)->remoteAddress() << ":" << (*it)->remotePort() << endl;
			}

		// commands sent to clients => start with "do "
		} else if (input.compare(0, 3, "do ") == 0) {
			for (vector<TCPSocket*>::iterator it = clients.begin(); it != clients.end(); ++it) {
				(*it)->Send(input.substr(3));
			}
		}

		cout << endl;
		getline(cin, input);
	}

	// Close the server before exiting the program.
	tcpServer.Close();

	return 0;
}
