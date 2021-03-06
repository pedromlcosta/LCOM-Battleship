#include "keyboard_mouse.h"
#include "timer.h"
#include "game.h"
#include "button.h"
#include "battleship.h"
#include "bitmap.h"
#include "highscore.h"
#include "graphics.h"
#include "MainMenu.h"

Battleship* startBattleship() {

	// Criar um "objeto" de jogo "battleship"
	Battleship* battle = (Battleship*) malloc(sizeof(Battleship));

	// Subscrever dispositivos
	battle->IRQ_SET_KEYBOARD = kbd_subscribe_int();
	battle->IRQ_SET_MOUSE = mouse_subscribe_int();
	battle->IRQ_SET_TIMER = timer_subscribe_int();

	getMouse();

	// Por frequencia do timer a 60
	timer_set_square(0, 60);

	battle->highscore_winner = -1;

	// Inicializar Variaveis
	// currentState inicial e o menu inicial
	battle->kb_code = KEY_NONE;
	battle->currentState = MAIN_MENU_STATE;
	battle->state = newMainMenuState(battle);


	// finish initialization
	battle->done = 0;

	return battle;
}

void updateBattleship(Battleship* battleship) {
	int ipc_status, r;
	message msg;

	if ((r = driver_receive(ANY, &msg, &ipc_status)) != 0) {
		printf("driver_receive failed with: %d", r);
	}

	if (is_ipc_notify(ipc_status)) // receive notification of interrupt request. returns true if msg received is notification and false otherwise
			{
		switch (_ENDPOINT_P(msg.m_source)) // m_source contains the endpoint of the msg and _ENDPOINT extracts the process identifier from process's endpoint
		{
		case HARDWARE:
			if (msg.NOTIFY_ARG & battleship->IRQ_SET_MOUSE) {
				updateMouse();
			}

			if (msg.NOTIFY_ARG & battleship->IRQ_SET_TIMER) {
				if (battleship->timer_cnt % (60 / FPS) == 0) {
					battleship->timer_ticked = !battleship->timer_ticked;
				}

				battleship->timer_cnt++;
				if (battleship->timer_cnt == 60) {
					battleship->timer_cnt = 0;
				}
			}

			if (msg.NOTIFY_ARG & battleship->IRQ_SET_KEYBOARD) {
				if (battleship->kb_code == 0xE0) {
					battleship->kb_code = battleship->kb_code << 8; //2 Bytes Makecode
					battleship->kb_code |= handler_kbd_asm();
				} else
					battleship->kb_code = handler_kbd_asm();
			}

			break;

		default:
			break;
		}
	} else {
		printf("Any interrupt received\n"); // Any interrupt received, so anything to do
	}
//	if (battleship->kb_code == KEY_ESC) {
//		battleship->done = 1;
//	}

	updateCurrentState(battleship);
}

void drawBattleship(Battleship* battleship) {

	switch (battleship->currentState) {

	case MAIN_MENU_STATE:
		drawMainMenuState(battleship);
		drawMouse();
		break;
	case GAME_PLAY_SETSHIP_STATE:
		cleanBufferSec();
		drawPlaySetship(battleship);
		updateBufferSec();
		break;
	case GAME_PLAY_STATE:
		cleanBufferSec();
		drawGame(battleship);
		updateBufferSec();
		break;
	case HIGHSCORE_STATE:
		cleanBufferSec();
		drawHighscore(battleship);
		updateBufferSec();
		break;

	default:
		break;
	}

}

void stopBattleship(Battleship* battleship) {

	deleteCurrentState(battleship);

	if (mouse_unsubscribe_int() != OK) {
		printf("Unsubscribe mouse failed");
	}

	kbd_unsubscribe_int();

	if (timer_unsubscribe_int() != OK) {
		printf("Unsubscribe timer failed");
	}

	deleteMouse();

	free(battleship);

	vg_exit();
}

void changeState(Battleship* battleship, State programState) {
//Apagar o state atual -> tem um switch que chama a funcao deleteNomeState
	deleteCurrentState(battleship);
//Fazer set ao state atual para aquele que se queria
	battleship->currentState = programState;
//Dependendo do estado que estamos a querer, vai-se criar esse estado
	battleship->kb_code = KEY_NONE;

	switch (battleship->currentState) {

	case MAIN_MENU_STATE:
		battleship->state = (MainMenuState *) newMainMenuState(battleship);
		break;
	case GAME_PLAY_SETSHIP_STATE:
		battleship->state = newPlaySetship(battleship);
		break;
	case GAME_PLAY_STATE:
		battleship->state = newGame(battleship);
		break;
	case HIGHSCORE_STATE:
		battleship->state = newHighscore(battleship);
		break;
	case EXIT_STATE:
		battleship->done = 1;
		break;
	default:
		break;
	}

}

//Esta funcao verifica se e necessario chamar changeState e chama esta funcao, se necessario, consoante
//certas condicoes
void updateCurrentState(Battleship* battleship) {
	State statetochange;

	switch (battleship->currentState) {
	case MAIN_MENU_STATE:
		statetochange = updateMainMenuState(battleship);
		if (((MainMenuState *) battleship->state)->done) {
			changeState(battleship, statetochange);
		}
		break;
	case GAME_PLAY_SETSHIP_STATE:
		statetochange = updatePlaySetship(battleship);
		if (((SetShipState *) battleship->state)->done) {
			changeState(battleship, statetochange);
		}
		break;
	case GAME_PLAY_STATE:
		statetochange = updateGame(battleship);
		if (((GameState *) battleship->state)->done) {
			changeState(battleship, statetochange);
		}
		break;
	case HIGHSCORE_STATE:
		statetochange = updateHighscore(battleship);
		if (((Highscore_State *) battleship->state)->done) {
			changeState(battleship, statetochange);
		}
		break;
	default:
		break;
	}

}

void deleteCurrentState(Battleship* battleship) {
	switch (battleship->currentState) {

	case MAIN_MENU_STATE:
		deleteMainMenuState(battleship);
		break;
	case GAME_PLAY_SETSHIP_STATE:
		deletePlaySetship(battleship);
		break;
	case GAME_PLAY_STATE:
		deleteGame(battleship);
		break;
	case HIGHSCORE_STATE:
		deleteHighscore(battleship);
		break;

	default:
		break;
	}
}
