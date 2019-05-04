#ifndef SANTA_H
#define SANTA_H

#include "synch.h"

class Santa{
	public:
		Santa();
		~Santa();
		void inicializar();
		int slave(int goblin);
		void procrastinate(int goblin, int taller_actual);
		int tryToWork(int goblin);
		void leaveWork(int goblin, int taller_actual);
		void print();
	private:
		enum{PROCRASTINATING, WORKING} current_status[4]; //Cuatro duendes con 2 estados posibles.
		Lock* dp;
		Condition* miedoso; //Conditional statement para cuando miedoso quiere entrar a trabajar y está solo.
		Condition* taller_no_disponible; //Conditional statement para cuando tratan de usar el taller y esta ocupado.
		int taller[2] = {0}; //2 talleres inicializados en 0.

};

#endif