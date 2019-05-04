// Luis Antonio Valverde Cruz B57414
// Clase monitor Santa para threadtest.
//0 = TOM
//1 = JERRY
//2 = GRUÑÓN
//3 = MIEDOSO

#include "santa.h"

Santa::Santa(){
	inicializar();
}

Santa::~Santa(){
	delete miedoso;
	delete taller_no_disponible;
	delete dp;
}

void Santa::inicializar(){
	dp = new Lock("Goblin");
	miedoso = new Condition("miedoso_entra");
	taller_no_disponible = new Condition("taller_no_disponible");
	for(int i = 0; i < 4; ++i){
		current_status[i] = PROCRASTINATING; //inicializa el estado de los 4 duendes.
	}
}


int Santa::slave(int goblin){ //manda a trabajar al duende específico.
	//printf("%d hizo Acquire en slave\n", goblin);
	int taller_actual = tryToWork(goblin); //intenta mandar al duende a trabajar.
	if(current_status[goblin] != WORKING){ //si no logró ponerlo a trabajar
		if(goblin == 3){ //y es miedoso.
			dp->Acquire();
			miedoso->Wait(dp); //espera a que haya alguien con quien trabajar.
			dp->Release();
			taller_actual = tryToWork(goblin); //lo manda a trabajar cuando hay alguien.
		}
		else{ //si es alguno de los otros
			dp->Acquire();
			taller_no_disponible->Wait(dp); //significa que no hay talleres disponibles.
			dp->Release();
			taller_actual = tryToWork(goblin); //lo manda a trabajar cuando hay taller.
		}
	}
	return taller_actual; //retorna el taller en el que se encuentra el duende.
}

void Santa::procrastinate(int goblin, int taller_actual){ //intenta sacar a un duende del taller.
	leaveWork(goblin, taller_actual);
	if(current_status[goblin] != PROCRASTINATING){ //si no logró sacarlo
		dp->Acquire();
		miedoso->Wait(dp); //es porque está con miedoso.
		dp->Release();
		leaveWork(goblin, taller_actual); //intenta de nuevo sacarlo.
	}
}

int Santa::tryToWork(int goblin){ //esta es realmente la zona crítica.
	dp->Acquire();
	int taller_actual = -1;
	switch(goblin){
		case 0: case 1: //Tom y Jerry
			if(taller[0] == 0){
				++taller[0];
				current_status[goblin] = WORKING;
				taller_actual = 0;
				miedoso->Signal(dp); //Miedoso puede entrar con él.
			} else if(taller[1] == 0){
				++taller[1];
				current_status[goblin] = WORKING;
				taller_actual = 1;
				miedoso->Signal(dp);
			} else{
				//Tiene que esperar taller disponible.
			}
			break;
		case 2: //Gruñon
			if(taller[0] == 0){
				taller[0] = 2;
				current_status[goblin] = WORKING;
				taller_actual = 0;
			} else if(taller[1] == 0){
				taller[1] = 2;
				current_status[goblin] = WORKING;
				taller_actual = 1;
			} else{
				//Tiene que esperar taller disponible.
			}
			break;
		case 3: //Miedoso
			if(taller[0] == 1){
				++taller[0];
				current_status[goblin] = WORKING;
				taller_actual = 0;
			} else if(taller[1] == 1){
				++taller[1];
				current_status[goblin] = WORKING;
				taller_actual = 1;
			} else {
				//Tiene que esperar a alguien con quien trabajar.
			}
			break;
		default:
			perror("TryToWork: Duende inválido: ");
			break;
	}
	dp->Release();
	return taller_actual;
}

void Santa::leaveWork(int goblin, int taller_actual){
	dp->Acquire();
	switch(goblin){
		case 0: case 1: //Tom y Jerry
			if(taller[taller_actual] == 1){
				--taller[taller_actual];
				current_status[goblin] = PROCRASTINATING;
				taller_no_disponible->Signal(dp);
			} else if(taller[taller_actual] == 2){
				//espera a miedoso.
			}
			break;
		case 2: //Gruñon
			taller[taller_actual] = 0;
			current_status[goblin] = PROCRASTINATING;
			taller_no_disponible->Signal(dp);
			break;
		case 3: //Miedoso
			--taller[taller_actual];
			current_status[goblin] = PROCRASTINATING;
			miedoso->Signal(dp);
			break;
		default:
			perror("LeaveWork: Duende inválido: ");
			break;
	}
	dp->Release();
}

void Santa::print() {
    char* name = new char[25];
    for ( int i = 0; i < 4; ++i ) {
    	switch(i){
            case 0:
                strcpy(name, "Tom");
                break;
            case 1:
                strcpy(name, "Jerry");
                break;
            case 2:
                strcpy(name, "Gruñon");
                break;
            case 3:
                strcpy(name, "Miedoso");
                break;
            default:
                break;
        }
        printf( " %s is %s \n", name, (current_status[i]==PROCRASTINATING)?"Vacationing":"Working");
    }
    delete name;

}