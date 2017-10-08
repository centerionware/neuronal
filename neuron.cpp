#include <vector>
#include <iostream>
#include <algorithm>
#include "tools.h"

#include <unistd.h>
#include <termios.h>
//#include <boost/shared_ptr.hpp>

// un-line buffer stdin and optionally set non-blocking
 void set_stdin(const bool block /* false for non-blocking */)
 {
	 const int fd = fileno(stdin);
	 termios flags;
	 if (tcgetattr(fd,&flags)<0) {} //... /* handle error */}
	 flags.c_lflag &= ~ICANON; // set raw (unset canonical modes)
	 flags.c_cc[VMIN] = block; // i.e. min 1 char for blocking, 0 chars for non-blocking
	 flags.c_cc[VTIME] = 0; // block if waiting for char
	 if (tcsetattr(fd,TCSANOW,&flags)<0) {}// ... /* handle error */}
 }

bool checkifdata (FILE *f)
{
	int fno = fileno (f);
	fd_set fdset;
	FD_ZERO (&fdset);		// note: fdset must be referenced here and below
	FD_SET (fno, &fdset);
	struct timeval       timeout;
	timeout.tv_sec  = 0;
	timeout.tv_usec = 60;

	return select (fno + 1, &fdset, NULL, NULL, &timeout);	
}


///
///// Replace all occurences of a substring in a string with another
///// string, in-place.
/////
///// @param s String to replace in. Will be modified.
///// @param sub Substring to replace.
///// @param other String to replace substring with.
/////
///// @return The string after replacements.
inline std::string & replacein(std::string &s, const std::string &sub, const std::string &other)
{

	size_t b = 0;
	for (;b < s.size(); b++)
	{
		b = s.find(sub, b);
		if (b == s.npos) break;
		s.replace(b, sub.size(), other);
		b += other.size();
	}
	return s;
}


class neuron;
template<class T>
class connection{
	public:
		T* ConnectedNeuron;
		long int amount;
		long int forgetTime;
		bool flagDelete;
		connection(T* in) {
			amount = 10;		
			forgetTime = 30;
			flagDelete = false;
			ConnectedNeuron = in;
			in->connectionsAmount++;
		}
		void reset() {
			if(forgetTime < 30) 
			forgetTime = 30;
			else forgetTime += 30;
			flagDelete = false;
		}
};

int processDepth;
int searchdepth;

neuron * lastCreated;

neuron * BASENEURON;


class neuron {
	public:
	std::vector<connection<neuron>* > connections;
	bool connfound;
	connection<neuron> * lastcon;
	neuron * lastnewconnection;
	bool baseNeuron;

	std::string response;
	int connectionsAmount;
	neuron() {
		this->connectionsAmount = 1;
		this->baseNeuron = false;

		response = "";
	}

	void createConnection(neuron *nl) {
		this->connfound = false;
		this->lastnewconnection = nl;

		std::vector<connection<neuron> * >::iterator begin = connections.begin(), end = connections.end();
		while(begin != end) { this->findConnection(*begin); begin++; };
		if(!connfound) {
			connection<neuron> * c =  new connection<neuron>(nl);
			connections.push_back(c);
		} else {
			lastcon->amount++;
			lastcon->reset();

		}

	};
	void findConnection(connection<neuron> *c) {
		if(c->ConnectedNeuron == lastnewconnection) {
			this->connfound = true;
			this->lastcon = c;
		}
	};
	bool processing;
	void baseProcess() {
		processDepth++;
		processing = true;
		std::vector<connection<neuron> * >::iterator begin = connections.begin(), end = connections.end();
		while(begin != end) 
		{ 
			connection<neuron>  * iter = (*begin);

			iter->ConnectedNeuron->process(iter, this); 
			begin++;
	       	};
//		void *lmc = lastCreated;
//		boost::shared_ptr<neuron> *llc = (boost::shared_ptr<neuron>*)llc;
//		neuron *lc = &(*(*llc));
		if(lastCreated != NULL) {
			begin = lastCreated->connections.begin(), end = lastCreated->connections.end();
			while(begin != end) { (*begin)->ConnectedNeuron->process(*begin, lastCreated); begin++; };
		}

		if(this->baseNeuron || &(*lastCreated) == this) {
			if(response != "") {
				std::cout << "Response: "  << response << "\n";
				response = "";
			}
			processDepth = 0;
		} else
			processDepth--;
		processing = false;
	}
	neuron *ptop;
	void process(connection<neuron> *from, neuron *top) {
//		std::cout << "Process\n";
		if(processDepth > 5) return;
		lastcon = from;
		this->response = "";
		ptop = top;
		std::vector<connection<neuron> * >::iterator begin = from->ConnectedNeuron->connections.begin(), end = from->ConnectedNeuron->connections.end();
		while(begin != end) { 
			lastcon = from;
			processConnection(from); begin++; 
		};

		from->ConnectedNeuron->forgetsome();
		forgetsome();
		top->forgetsome();
//		top->response.append(from->ConnectedNeuron->response);
//		top->response = "";
		if(!processing) baseProcess();
//		from->ConnectedNeuron->baseProcess();
	};
	bool isConnected(neuron * nin) {
		std::vector<connection<neuron> * >::iterator begin = connections.begin(), end = connections.end();
		for(;begin != end; begin++) {
		connection<neuron> * c = *begin;
			if(c->ConnectedNeuron == nin) return true;
		}
		return false;
	}
	void processConnection(connection<neuron> *c) {
		if(c->ConnectedNeuron->isConnected(lastcon->ConnectedNeuron)) {
			c->amount++;
			if(c->amount > 10) {
				if(ptop->response != "");
					ptop->response.append(" ");
				ptop->response.append(c->ConnectedNeuron->word);
			}
		}
	};

	void forgetsome() {
		std::vector<connection<neuron> * >::iterator begin = connections.begin(), end = connections.end();
		while(begin != end) { this->processForget(*begin); begin++; };
		for(size_t c = 0; c < connections.size(); c++) {
			if(connections[c]->flagDelete == true ) {
				connection<neuron> *l = *(connections.begin()+c);
				if(l->ConnectedNeuron->connectionsAmount == 1) {
					std::cout << "Neuron destroyed!\n";
					delete l->ConnectedNeuron;
					delete l;

				}
				else { l->ConnectedNeuron->connectionsAmount--;
					delete l;
				} 
				connections.erase(connections.begin()+c);
				c--;
			}
		}
	};
	void boostConnection(neuron * in) {
		for(int c = 0; c < connections.size(); c++) 
			if (connections[c]->ConnectedNeuron == in) {
		 		std::cout << "Boosted connection<neuron>to " <<word << " from " << in->word<<"\n";     
			connections[c]->amount++;
		}
	}
	void processForget(connection<neuron> * c) {
		if(c->forgetTime == 0) c->amount--;
		else c->forgetTime--;
		if(c->amount == 0) c->flagDelete = true;
	}
	std::string word; // each neuron contains one word in it.
	
	neuron * isFound(std::string word_f) {
		for(size_t c = 0; c < connections.size(); c++ ){
			if(connections[c]->ConnectedNeuron->word.compare(word_f) == 0) {
				return (connections[c]->ConnectedNeuron);

			}
		}
		if(word_f == word) return this;
		return NULL;
	}
	neuron* findWord(std::string word_f) {
		searchdepth++;
		if(searchdepth > 100000) {
			return NULL;
		}
		
		for(size_t c = 0; c < connections.size(); c++ ){
			if(connections[c]->ConnectedNeuron->isFound(word_f)) {
				searchdepth--;
				return connections[c]->ConnectedNeuron;
			};
			neuron *t = NULL;
			t = connections[c]->ConnectedNeuron->findWord(word_f);
			if(t != NULL) {
				searchdepth--;
				return t;
			}
		}
		searchdepth--;
		return NULL;
	}
	neuron * getBase() {
//		if(!this->baseNeuron) {
//		       	void *base = this->parent->findWord(this->word);	
//			boost::shared_ptr<neuron> ptr = *(boost::shared_ptr<neuron>*)base;
//			while(ptr->baseNeuron == false) ptr = ((ptr)->parent);
//			return ptr;
//		} else {
			return BASENEURON;
//		}
	}
	connection<neuron> *findConnection( neuron * in) {
		std::vector<connection<neuron> * >::iterator begin = connections.begin(), end = connections.end();
		while(begin != end) 
		{ 
			if((*begin)->ConnectedNeuron == in) {
//				connection<neuron> *c = &(*(*begin));
				return *begin;
			}
			begin++;
		};
		return NULL;
	}
	void learnsome(std::string input) {
		input = replacein(input, "\n", "");
		std::vector<std::string> arr = explode(" ",input);
		if(this->baseNeuron) lastCreated = NULL;
		for(int c = 0; c < arr.size(); c++) {
			arr[c] = replacein(arr[c], " ", "");
			neuron *t = getBase()->findWord(arr[c]);
			if( t == NULL) {
				neuron* n;

				if(lastCreated == NULL) {
					n = new neuron();
					(BASENEURON)->connections.push_back( new connection<neuron>(n));
				} else {
				       	n = new neuron();
					(lastCreated)->connections.push_back( new connection<neuron>(n));
				}
				std::cout << "New neuron!\n";
				lastCreated = n;
				n->word = arr[c];
//				n->parent->connections.push_back(boost::shared_ptr<connection<neuron> >(new connection<neuron>(n)));
			} else {
				neuron * v = t;
				if(lastCreated) {
					neuron * lc = lastCreated;
					if( ((lc))->word != arr[c]) {
						connection<neuron>* cont = lc->findConnection(v);
						if(cont != NULL) { 
							cont->amount++;//t->boostConnection(cont->ConnectedNeuron);
						} else
						lc->connections.push_back( new connection<neuron>(v));
					}
				} else {
//					(v)->parent->boostConnection(v);
				}
				lastCreated = v;
			}
		}
		
	}
};
#include <time.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
int main() {
	processDepth = 0;
	searchdepth = 0;

	lastCreated = NULL;
	//hasLastCreated = false;
	set_stdin(false);

	neuron *baseNeuron = new neuron();

	baseNeuron->baseNeuron = true;
	baseNeuron->word = " ";
	std::string input = "";

	BASENEURON = baseNeuron;
	time_t lasttime = 0;
	while(true) {

		try {
			if(checkifdata(stdin)) {
				char g = fgetc( stdin);
				input += g;

			}
		}catch(...) {}

		if(input.find("\n") != std::string::npos && input.compare("\n") != 0 ){
			baseNeuron->learnsome(input);
			input = "";
		} if(input.compare("\n") == 0) input = "";
		if(lasttime != time(NULL)) {
			baseNeuron->baseProcess();
			lasttime = time(NULL);
		}
		int high=30000;
		int low=300;


		usleep(rand() % (high - low + 1) + low);
	}
	// get input, pass it to learnsome on baseNeuron.
	// Process base neuron once every second
	// add input when enter is pressed
}

