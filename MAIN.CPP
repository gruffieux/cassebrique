#include <dxerror.h>
#include <cinput.h>
#include <cmsgreporter.h>
#include <cconfigfile.h>
#include <ckeyboardlistener.h>
#include <csound.h>
#include <canimation.h>
#include <ctileengine.h>
#include "cracket.h"
#include "ccapsule.h"
#include "cball.h"
#include "resource.h"
#include "CBRes/resource.h"

/*
TODO:

Bugs:
- Parfois, lors d'un rebond bizrarre, la balle traverse les murs
*/

//////////////////////////////
// Prototypes des fonctions //
//////////////////////////////

void CheckUpdate();
void DestroyAll();
void DestroyBall(Ball *pBall);
void DestroyCapsule(Capsule *pCapsule);
void DestroyDirectDraw();
void DestroyDirectInput();
void DestroyDirectMusic();
void DrawBall();
void InitDirectDraw();
void InitDirectInput();
void InitDirectMusic();
void InitObjects();
void InitWindow(HINSTANCE hInst, int nCmdShow);
bool LoadConfig();
void MakeHighScore();
void NewGame();
void PlayScene();
bool ProcessBrickCollision(CardinalPoint *BrickEdge, Axe *BrickEdgeID);
bool ProcessKbInput();
void ProcessMouseInput();
void ReleaseCapsule(int x, int y, double speed, EFFECT effect);
void RestoreDirectDraw();
void SetBrickEdge(CardinalPoint *BrickEdge, Axe *BrickEdgeID, int x, int y);
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

////////////////////////
// Variables globales //
////////////////////////

//Objets de base nécessaire à l'application
HWND hWndMain;
Display *Screen;
Input *Keyboard, *Mouse;

//Objets du jeu
ConfigFile Config;
KeyboardListener keybListener;
Sound *Music[MAX_MUSIC], *Noise[MAX_NOISE];
Surface *Background[MAX_MUSIC], *Text[MAX_TEXT];
TileEngine *Levels;
Racket *PlayerRacket;
List Balls, Capsules;
Clock TicTac, BallWaiter;

//Variables et structures
int SceneCount, MaxShoot, score, ScoreLevel, life, bonus, track;
bool BreakBarrier, wait, flickball, SafeGravity, reward, GameOver, restart, running, reinit;
short BrickRecovery[MAX_TILES_Y][MAX_TILES_X][MAX_LAYERS];
FILE *HighScoreFile;

///////////////////////////////
// Définitions des fonctions //
///////////////////////////////

void CheckUpdate()
{
	DWORD exitCode;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

	CreateProcess(NULL, "UpdateClient.exe CHECK", NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
	WaitForSingleObject(pi.hProcess, 15000);
	GetExitCodeProcess(pi.hProcess, &exitCode);

	switch (exitCode)
	{
	case 0:
		break;
	case 1:
		if (MessageBox(NULL, "Le programme doit fermer, voulez vous continuer? Relancez-le après la mise à jour.", "Mise à jour disponible!", MB_YESNO) == IDYES)
		{
			ShellExecute(NULL, "open", "UpdateClient.exe", "UPDATE", NULL, SW_SHOW);
			exit(0);
		}
		break;
	default:
		TerminateProcess(pi.hProcess, 0);
		MessageBox(NULL, "Impossible de vérifier la mise à jour, si le problème persiste, lancez le programme de mis à jour par vous-même.", "Erreur inconnue", MB_ICONERROR);
		break;
	}
}

void DestroyAll()
{
	int i;

	DestroyDirectDraw();
	DestroyDirectInput();
	DestroyDirectMusic();

	for (i = 0; i < MAX_TEXT; i++)
		delete Text[i];

	delete PlayerRacket;

	for (i = 0; i < Balls.GetElementCount(); i++)
		DestroyBall(Ball::getBallElement(i, &Balls));

	for (i = 0; i < Capsules.GetElementCount(); i++)
		DestroyCapsule(Capsule::getCapsuleElement(i, &Capsules));

	delete Levels;
	delete Screen;
	delete Keyboard;
	delete Mouse;
}

void DestroyBall(Ball *pBall)
{
	pBall->destroyAni();
	Balls.RemoveElement(pBall, true);
	pBall = NULL;
}

void DestroyCapsule(Capsule *pCapsule)
{
	pCapsule->destroyAni();
	Capsules.RemoveElement(pCapsule, true);
	pCapsule = NULL;
}

void DestroyDirectDraw()
{
	int i;

	//Les textes
	for (i = 0; i < MAX_TEXT; i++)
		Text[i]->Destroy();

	//Les fonds
	Background[0]->Destroy();
	Background[1]->Destroy();
	Background[2]->Destroy();
	Background[3]->Destroy();
	Background[4]->Destroy();
	Background[5]->Destroy();
	
	//Les briques
	Levels->GetTileData()->destroyAni();

	//La raquette
	PlayerRacket->destroyAni();

	//Les capsules
	for (i = 0; i < Capsules.GetElementCount(); i++)
		Capsule::getCapsuleElement(i, &Capsules)->destroyAni();

	//Les balles
	for (i = 0; i < Balls.GetElementCount(); i++)
		Ball::getBallElement(i, &Balls)->destroyAni();
	
	Screen->Destroy();
}

void DestroyDirectInput()
{
	Keyboard->Destroy();
	Mouse->Destroy();
}

void DestroyDirectMusic()
{
	int i;

	for (i = 0; i < MAX_MUSIC; i++)
		Music[i]->destroy();

	for (i = 0; i < MAX_NOISE; i++)
		Noise[i]->destroy();

	Sound::destroyPlayer();

	for (i = 0; i < MAX_MUSIC; i++)
	{
		delete Music[i];
		delete Background[i];
	}

	for (i = 0; i < MAX_NOISE; i++)
		delete Noise[i];
}

void DrawBall()
{
	int x, y;
	Ball *pBall;

	x = PlayerRacket->getPosition()->x + PlayerRacket->getDimension()->x / 2 - 10;
	y = PlayerRacket->getPosition()->y - 20;

	pBall = new Ball(x, y, 20, 20, PlayerRacket->GetSight()->GetAngle(), 0.3, 0.3, "Balle");

	pBall->addAni(IDB_BALL, "", 2, 100, 100, RGB(0, 0, 0), 270);
	pBall->SetMagnitude(6);

	Balls.AddElement(pBall);
}

void InitDirectDraw()
{
	Screen->SetDisplayMode();

	//Les fonds
	Screen->LoadBitmap(IDB_BACKGROUND, Background[0], 0, 270);
	Screen->LoadBitmap(IDB_BACKGROUND1, Background[1], 0, 270);
	Screen->LoadBitmap(IDB_BACKGROUND2, Background[2], 0, 270);
	Screen->LoadBitmap(IDB_BACKGROUND3, Background[3], 0, 270);
	Screen->LoadBitmap(IDB_BACKGROUND4, Background[4], 0, 270);
	Screen->LoadBitmap(IDB_BACKGROUND5, Background[5], 0, 270);
	
	//Les briques
	Levels->GetTileData()->addAni(IDB_LEVELS, "", 0, 0, 0, RGB(0, 0, 0), 270);

	//La raquette
	PlayerRacket->addAni(IDB_RACKET, "", 1, 100, 20, RGB(255, 255, 255), 270);
}

void InitDirectInput()
{
	Keyboard->CreateDevice(GUID_SysKeyboard, &c_dfDIKeyboard, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
	Mouse->CreateDevice(GUID_SysMouse, &c_dfDIMouse, DISCL_EXCLUSIVE | DISCL_FOREGROUND);
}

void InitDirectMusic()
{
	int i;

	Sound::initPlayer(hWndMain, DMUS_APATH_DYNAMIC_STEREO, 64, DMUS_AUDIOF_ALL);

	for (i = 0; i < MAX_MUSIC; i++)
	{
		Music[i] = new Sound();
		Background[i] = new Surface();
	}

	for (i = 0; i < MAX_NOISE; i++)
		Noise[i] = new Sound();

	//Charge les fichiers sons en mémoire
	Music[CLASSIC]->load("Sons/classic.mid", true);
	Music[COUNTRY]->load("Sons/country.mid", true);
	Music[FUNK]->load("Sons/funk.mid", true);
	Music[JAZZ]->load("Sons/jazz.mid", true);
	Music[REGGAE]->load("Sons/reggae.mid", true);
	Music[ROCK]->load("Sons/rock.mid", true);
	Noise[FALL]->load("Sons/fall.wav");
	Noise[GAMEOVER1]->load("Sons/gameover1.wav");
	Noise[GAMEOVER2]->load("Sons/gameover2.wav");
	Noise[GOODBYE]->load("Sons/goodbye.wav");
	Noise[IMPACT1]->load("Sons/impact1.wav");
	Noise[IMPACT2]->load("Sons/impact2.wav");
	Noise[IMPACT3]->load("Sons/impact3.wav");
	Noise[IMPACT4]->load("Sons/impact4.wav");
	Noise[PICKUP]->load("Sons/pickup.wav");
	Noise[SKIP]->load("Sons/skip.wav");
	Noise[YOUPI]->load("Sons/youpi.wav");
}

void InitObjects()
{
	Levels->GetMapFile()->SetFileName("levels.dat");
	Levels->GetScreenData()->dimension = Axe(Screen->GetCurrentMode()->GetWidth(), Screen->GetCurrentMode()->GetHeight());

	*PlayerRacket->getPosition() = Axe(Screen->GetCurrentMode()->GetWidth() / 2 - PlayerRacket->getDimension()->x / 2, Screen->GetCurrentMode()->GetHeight() - PlayerRacket->getDimension()->y);
}

void InitWindow(HINSTANCE hInst, int nCmdShow)
{
	WNDCLASS wc; //Structure contenant les informations de la fenêtre
	
	//----------Enregistrement d'une classe window----------

	//Assignation de valeure pour chaques champs de la structure wc
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInst;
	wc.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
	wc.hCursor = LoadCursor((HINSTANCE) NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "MainWndClass";

	//Enregistrement
	if (!RegisterClass(&wc))
		ReportDxError(S_FALSE, WIN32_, ERR_REGISTERCLASS);
	
	//----------Création de la fenêtre----------

	hWndMain = CreateWindow(wc.lpszClassName, "Casse Brique", WS_POPUPWINDOW, 0, 0, 1024, 768, (HWND) NULL, (HMENU) NULL, hInst, (LPVOID) NULL);

	if (!hWndMain)
		ReportDxError(S_FALSE, WIN32_, ERR_CREATEWIN);

	ShowWindow(hWndMain, nCmdShow); //Afficher la fenêtre
	UpdateWindow(hWndMain);
}

bool LoadConfig()
{
	int i;
	bool b;

	if (!Config.Open(true, false, false))
		return false;

	Config.ReadGraphic(Screen->GetCurrentMode());
	Config.ReadKeys();

	b = Config.PlayMusic();

	for (i = 0; i < MAX_MUSIC; i++)
		if (b)
			Music[i]->setMuted(0);
		else
		{
			Music[i]->setMuted(2);
			Music[i]->mute();
		}

	b = Config.PlaySounds();

	for (i = 0; i < MAX_NOISE; i++)
		if (b)
			Noise[i]->setMuted(0);
		else
		{
			Noise[i]->setMuted(2);
			Noise[i]->mute();
		}

	Config.Close();

	if (track != -1 && !Music[track]->getMuted() && !Music[track]->isPlaying())
		Music[track]->play(DMUS_SEGF_DEFAULT, false);

	return true;
}

void MakeHighScore()
{
	if (!fopen_s(&HighScoreFile, "highscore.dat", "rb"))
		if (score > _getw(HighScoreFile))
		{
			fclose(HighScoreFile);
			if (!fopen_s(&HighScoreFile, "highscore.dat", "wb"))
			{
				_putw(score, HighScoreFile);
				fclose(HighScoreFile);
			}
		}
		else
			fclose(HighScoreFile);
}

void NewGame()
{
	int i;

	// Initialise les valeurs
	SceneCount = MAX_SCENE - 1;
	MaxShoot = score = bonus = 0;
	ScoreLevel = 1;
	life = 3;
	BreakBarrier = wait = flickball = SafeGravity = reward = GameOver = false;

	for (i = 0; i < Balls.GetElementCount(); i++)
		DestroyBall(Ball::getBallElement(i, &Balls));

	for (i = 0; i < Capsules.GetElementCount(); i++)
		DestroyCapsule(Capsule::getCapsuleElement(i, &Capsules));

	// Vérifie le fichier higscore
	if (fopen_s(&HighScoreFile, "highscore.dat", "rb"))
	{
		if (!fopen_s(&HighScoreFile, "highscore.dat", "wb"))
		{
			_putw(0, HighScoreFile);
			fclose(HighScoreFile);
		}
	}
	else
		fclose(HighScoreFile);

	// Initialise la carte
	Levels->LoadMap(0);

	// Joue un nouveau morceau
	Sound::stop(DMUS_SEGF_DEFAULT);
	track = Clock::random(0, MAX_MUSIC - 1);
	Music[track]->play(DMUS_SEGF_DEFAULT, false);

	PlayerRacket->SetCurrentEffect(NO_EFFECT);
	PlayerRacket->GetSight()->SetAngle(PI + PI / 4);

	restart = true;
}

void PlayScene()
{
	int i, j, BreakableBrickCount, UnbreakableBrickCount, BarrierCount, x, y;
	Axe BreakableBrickEdgeID[MAX_BREAKABLE_BRICK], UnbreakableBrickEdgeID[MAX_UNBREAKABLE_BRICK];
	CardinalPoint RacketEdge[1], *BallEdge, BreakableBrickEdge[MAX_BREAKABLE_BRICK], UnbreakableBrickEdge[MAX_UNBREAKABLE_BRICK];
	Str Title;
	Capsule *pCapsule;
	Ball *pBall;

	BreakableBrickCount = UnbreakableBrickCount = BarrierCount = 0;
	if (SceneCount)
		Levels->GetScreenData()->MaxCamera.north = Levels->GetScreenData()->VisibleTileCount.y * Levels->GetTileData()->getDimension()->y * (SceneCount - 1);
	else
		BarrierCount = 1;

	//Affichage du fond et des briques
	Screen->Blit(0, 0, Background[track], NULL, NULL, DDBLT_WAIT);
	Levels->RenderMap(0);

	if (restart)
	{
		Levels->GetScreenData()->Camera.SetAngle(PI / 2);
		Levels->GetScreenData()->Camera.SetMagnitude(24);
		if (Levels->GetScreenData()->Camera.getPosition()->y < Levels->GetScreenData()->MaxCamera.south)
			Levels->Scroll(NULL, NULL, true, false);
		else
		{
			restart = false;
			Levels->GetScreenData()->Camera.SetMagnitude(0);
		}
	}

	for (x = 0; x < Levels->GetMapData()->TileCount.x; x++)
		if (BreakBarrier)
		{
			for (y = SceneCount * Levels->GetScreenData()->VisibleTileCount.y; y < SceneCount * Levels->GetScreenData()->VisibleTileCount.y + Levels->GetScreenData()->VisibleTileCount.y; y++)
				//Détruit les briques de la barrière
				if (Levels->GetMapData()->TileID[y][x][0] == 1)
				{
					BrickRecovery[y][x][0] = Levels->GetMapData()->TileID[y][x][0];
					Levels->GetMapData()->TileID[y][x][0] = 0;
					BarrierCount++;
				}
			for (y = (SceneCount - 1) * Levels->GetScreenData()->VisibleTileCount.y; y < SceneCount * Levels->GetScreenData()->VisibleTileCount.y + Levels->GetScreenData()->VisibleTileCount.y; y++)
				//Stocke les briques incassables
				if ((Levels->GetMapData()->TileID[y][x][0] == 1) | (Levels->GetMapData()->TileID[y][x][0] == 2))
				{
					SetBrickEdge(&UnbreakableBrickEdge[UnbreakableBrickCount], &UnbreakableBrickEdgeID[UnbreakableBrickCount], x, y);
					UnbreakableBrickCount++;
				}
			for (y = (SceneCount - 1) * Levels->GetScreenData()->VisibleTileCount.y; y < (SceneCount - 1) * Levels->GetScreenData()->VisibleTileCount.y + Levels->GetScreenData()->VisibleTileCount.y; y++)
				//Stocke les briques cassables
				if (Levels->GetMapData()->TileID[y][x][0] >= 3)
				{
					SetBrickEdge(&BreakableBrickEdge[BreakableBrickCount], &BreakableBrickEdgeID[BreakableBrickCount], x, y);
					BreakableBrickCount++;
				}
		}
		else
			for (y = SceneCount * Levels->GetScreenData()->VisibleTileCount.y; y < SceneCount * Levels->GetScreenData()->VisibleTileCount.y + Levels->GetScreenData()->VisibleTileCount.y; y++)
			{
				//Reconstruction de la barrière
				if (BrickRecovery[y][x][0] == 1)
				{
					Levels->GetMapData()->TileID[y][x][0] = BrickRecovery[y][x][0];
					BrickRecovery[y][x][0] = 0;
				}
				//Reconstruction des briques du mur détruites
				if ((PlayerRacket->GetCurrentEffect() == REBUILD1) & (BrickRecovery[y][x][0] == 2))
				{
					Levels->GetMapData()->TileID[y][x][0] = BrickRecovery[y][x][0];
					BrickRecovery[y][x][0] = 0;
				}
				//Reconstruction de toutes les briques détruites
				if ((PlayerRacket->GetCurrentEffect() == REBUILD2) & (BrickRecovery[y][x][0] >= 2))
				{
					Levels->GetMapData()->TileID[y][x][0] = BrickRecovery[y][x][0];
					BrickRecovery[y][x][0] = 0;
				}
				//Compte les briques de la barrière
				if (Levels->GetMapData()->TileID[y][x][0] == 1)
					BarrierCount++;
				//Stocke les briques incassables
				if ((Levels->GetMapData()->TileID[y][x][0] == 1) | (Levels->GetMapData()->TileID[y][x][0] == 2))
				{
					SetBrickEdge(&UnbreakableBrickEdge[UnbreakableBrickCount], &UnbreakableBrickEdgeID[UnbreakableBrickCount], x, y);
					UnbreakableBrickCount++;
				}
				//Stocke les briques cassables
				if (Levels->GetMapData()->TileID[y][x][0] >= 3)
				{
					SetBrickEdge(&BreakableBrickEdge[BreakableBrickCount], &BreakableBrickEdgeID[BreakableBrickCount], x, y);
					BreakableBrickCount++;
				}
			}

	if (!PlayerRacket->getRemoved())
	{
		RacketEdge[0].west = PlayerRacket->getPosition()->x;
		RacketEdge[0].east = PlayerRacket->getPosition()->x + PlayerRacket->getDimension()->x;
		RacketEdge[0].north = PlayerRacket->getPosition()->y;
		RacketEdge[0].south = PlayerRacket->getPosition()->y + PlayerRacket->getDimension()->y;
		switch (PlayerRacket->GetCurrentEffect())
		{
		case REBUILD1:
			PlayerRacket->SetCurrentEffect(NO_EFFECT);
			break;
		case REBUILD2:
			PlayerRacket->SetCurrentEffect(NO_EFFECT);
			break;
		case SKIPLEVEL:
			if (SceneCount)
				BreakBarrier = true;
			break;
		case CANCEL:
			MaxShoot = bonus = 0;
			flickball = false;
			for (i = 0; i < Balls.GetElementCount(); i++)
			{
				pBall = Ball::getBallElement(i, &Balls);
				if (pBall->getDimension()->x > 20)
					pBall->getDimension()->x--;
				if (pBall->getDimension()->y > 20)
					pBall->getDimension()->y--;
				if (pBall->GetMagnitude() > 6)
					pBall->DecreaseMagnitude(6);
				if (pBall->GetMagnitude() < 6)
					pBall->IncreaseMagnitude(6);
			}
			break;
		}
		PlayerRacket->runAnimations();
		PlayerRacket->Move();
		for (i = 0; i < UnbreakableBrickCount; i++)
			if (PlayerRacket->GetCurrentEffect() == SPRING)
				PlayerRacket->GenerateCollision(&UnbreakableBrickEdge[i], UnbreakableBrickEdge, UnbreakableBrickCount, true, true);
			else
				PlayerRacket->GenerateCollision(&UnbreakableBrickEdge[i], UnbreakableBrickEdge, UnbreakableBrickCount, true, false);
	}

	for (i = 0; i < Capsules.GetElementCount(); i++)
	{
		pCapsule = Capsule::getCapsuleElement(i, &Capsules);
		if (pCapsule != NULL && !pCapsule->getRemoved())
		{
			pCapsule->setVisibleZone();
			pCapsule->runAni(0, pCapsule->GetEffect());
			pCapsule->Move();
			if (PlayerRacket->collision(pCapsule->getPosition()->x, pCapsule->getPosition()->x + pCapsule->getDimension()->x, pCapsule->getPosition()->y, pCapsule->getPosition()->y + pCapsule->getDimension()->y))
			{
				Noise[PICKUP]->play(DMUS_SEGF_SECONDARY);
				PlayerRacket->SetCurrentEffect(pCapsule->GetEffect());
				switch (PlayerRacket->GetCurrentEffect())
				{
				case FLICKBALL:
					flickball = true;
					break;
				case MULTIBALL:
					if (!MaxShoot)
						MaxShoot = (Clock::random(2, 9));
					break;
				case BONUS:
					if (!bonus)
						bonus = Clock::random(2, 5);
					break;
				}
				DestroyCapsule(pCapsule);
			}
			else
				if (pCapsule->getPosition()->y > Screen->GetCurrentMode()->GetHeight())
					DestroyCapsule(pCapsule);
		}
	}

	BallEdge = new CardinalPoint[Balls.GetElementCount()];

	for (i = 0; i < Balls.GetElementCount(); i++)
	{
		pBall = Ball::getBallElement(i, &Balls);
		if (pBall != NULL && !pBall->getRemoved())
		{
			if (SafeGravity) pBall->SetAngle(PI / 2);
			switch (PlayerRacket->GetCurrentEffect())
			{
			case SLOWBALL:
				pBall->DecreaseMagnitude(4);
				break;
			case BIGBALL:
				if (pBall->getDimension()->x < 100)
					pBall->getDimension()->x++;
				if (pBall->getDimension()->y < 100)
					pBall->getDimension()->y++;
				break;
			case SONICBALL:
				if (BarrierCount)
					pBall->IncreaseMagnitude(10);
				break;
			default:
				break;
			}
			pBall->setVisibleZone();	
			if (flickball)
				pBall->runAni(0, (double)2);
			else
			{
				pBall->runAni(0, (double)0);
				Animation::getAnimationElement(0, pBall->getAnimations())->GetFluidity()->setCounter(0);
			}
			pBall->Move(NULL, false, true);
			for (j = 0; j < Balls.GetElementCount(); j++)
			{
				BallEdge[j].west = Ball::getBallElement(j, &Balls)->getPosition()->x;
				BallEdge[j].east = Ball::getBallElement(j, &Balls)->getPosition()->x + Ball::getBallElement(j, &Balls)->getDimension()->x;
				BallEdge[j].north = Ball::getBallElement(j, &Balls)->getPosition()->y;
				BallEdge[j].south = Ball::getBallElement(j, &Balls)->getPosition()->y + Ball::getBallElement(j, &Balls)->getDimension()->y;
			}
			for (j = 0; j < Balls.GetElementCount(); j++)
				if ((Ball::getBallElement(j, &Balls)->getRemoved() == false) & (i != j))
					pBall->GenerateCollision(&BallEdge[j], BallEdge, Balls.GetElementCount(), true, true);
			pBall->GenerateCollision(&RacketEdge[0], RacketEdge, 1, true, true);
			if (!SafeGravity && BarrierCount)
			{
				for (j = 0; j < UnbreakableBrickCount; j++)
					if (ProcessBrickCollision(pBall->GenerateCollision(&UnbreakableBrickEdge[j], UnbreakableBrickEdge, UnbreakableBrickCount, true, true), &UnbreakableBrickEdgeID[j]))
					{
						if (PlayerRacket->GetCurrentEffect() == SPINBALL)
							pBall->InitSpin(40, (ROTATION)Clock::random(0, 1));
						break;
					}
				for (j = 0; j < BreakableBrickCount; j++)
					if (PlayerRacket->GetCurrentEffect() == SONICBALL)
					{
						if (ProcessBrickCollision(pBall->GenerateCollision(&BreakableBrickEdge[j], BreakableBrickEdge, BreakableBrickCount, false, false),&BreakableBrickEdgeID[j]))
							break;
					}
					else
						if (ProcessBrickCollision(pBall->GenerateCollision(&BreakableBrickEdge[j], BreakableBrickEdge, BreakableBrickCount, true, true), &BreakableBrickEdgeID[j]))
						{
							if (PlayerRacket->GetCurrentEffect() == SPINBALL)
								pBall->InitSpin(40, (ROTATION)Clock::random(0, 1));
							break;
						}
			}
			if (pBall->collision(PlayerRacket->getPosition()->x, PlayerRacket->getPosition()->x + PlayerRacket->getDimension()->x, PlayerRacket->getPosition()->y, PlayerRacket->getPosition()->y + PlayerRacket->getDimension()->y))
			{
				SafeGravity = false;
				if (PlayerRacket->GetCurrentEffect() == SPINBALL)
					pBall->InitSpin(40, (ROTATION)Clock::random(0, 1));
				else
				{
					if (PlayerRacket->GetSpinTotal() > 0)
						pBall->InitSpin(PlayerRacket->GetSpinTotal(), LIFT);
					if (PlayerRacket->GetSpinTotal() < 0)
						pBall->InitSpin(-PlayerRacket->GetSpinTotal(), CUT);
				}
				Noise[IMPACT2]->play(DMUS_SEGF_SECONDARY);
			}
			pBall->Spin();
			//Lorsque la balle est perdue
			if ((pBall->getPosition()->x + pBall->getDimension()->x < 0) | (pBall->getPosition()->x > Screen->GetCurrentMode()->GetWidth()) | (pBall->getPosition()->y + pBall->getDimension()->y < 0) | (pBall->getPosition()->y > Screen->GetCurrentMode()->GetHeight()))
			{
				MaxShoot = 0;
				SafeGravity = false;
				*pBall->getDimension() = Axe(20, 20);
				PlayerRacket->SetCurrentEffect(NO_EFFECT);
				Noise[GOODBYE]->play(DMUS_SEGF_SECONDARY);
				DestroyBall(pBall);
				//Lorsque toutes les balles sont perdues
				if (!Balls.GetElementCount())
				{
					bonus = 0;
					flickball = false;
					life--;
				}
			}
		}
		else
			pBall->SetAngle(PlayerRacket->GetSight()->GetAngle());
	}

	delete [] BallEdge;

	//Si il n'y a plus de briques
	if (!BreakableBrickCount)
		if (!SceneCount)
		{
			if (!GameOver)
			{
				GameOver = true;
				MakeHighScore();
				Sound::stop(DMUS_SEGF_DEFAULT);
				Noise[GAMEOVER1]->play(DMUS_SEGF_SECONDARY);
			}
		}
		else
			BreakBarrier = reward = true;

	//Si il n'y a plus de barrière
	if (!BarrierCount)
	{
		if (!Levels->GetScreenData()->Camera.GetMagnitude())
		{
			Noise[SKIP]->play(DMUS_SEGF_SECONDARY);

			if (Balls.GetElementCount())
				Levels->GetScreenData()->Camera.SetMagnitude(Ball::getBallElement(0, &Balls)->GetMagnitude());
			else
				Levels->GetScreenData()->Camera.SetMagnitude(6);

			Levels->GetScreenData()->Camera.SetAngle(3 * PI / 2);

			for (i = 0; i < Balls.GetElementCount(); i++)
				Ball::getBallElement(i, &Balls)->SetMagnitude(0);
		}

		Levels->Scroll(NULL, NULL, true, false);

		//Lorsque la caméra arrive au prochain niveau
		if (Levels->GetScreenData()->Camera.getPosition()->y <= (SceneCount - 1) * Levels->GetScreenData()->VisibleTileCount.y * Levels->GetTileData()->getDimension()->y)
		{
			//Ajoute une vie si le niveau a été entièrement détruit
			if (reward)
			{
				life++;
				reward = false;
				Noise[YOUPI]->play(DMUS_SEGF_SECONDARY);
			}
			SceneCount--;
			for (i = 0; i < Balls.GetElementCount(); i++)
				Ball::getBallElement(i, &Balls)->SetMagnitude(Levels->GetScreenData()->Camera.GetMagnitude());
			Levels->GetScreenData()->Camera.SetMagnitude(0);
			PlayerRacket->SetCurrentEffect(NO_EFFECT);
			BreakBarrier = false;
		}
	}

	//Si la musique courante est finie, jouer une autre
	if ((!Music[track]->getMuted() && Music[track]->finished()) | (track == FUNK && Music[FUNK]->getSeek() >= 100000) | (track == ROCK && Music[ROCK]->getSeek() >= 105700))
	{
		Sound::stop(DMUS_SEGF_DEFAULT);
		track = Clock::random(0, MAX_MUSIC - 1);
		Music[track]->play(DMUS_SEGF_DEFAULT, false);
	}

	//Attendre 3 seconde avant de lâcher une nouvelle capsule
	if (wait)
		if (!TicTac.goCounter(1, 0, 3, true))
			wait = false;

	//Ajoute une vie tous les 100 points
	if (score >= 1000 * ScoreLevel)
	{
		life++;
		ScoreLevel++;
		Noise[YOUPI]->play(DMUS_SEGF_SECONDARY);
	}

	if (!life)
	{
		if (!GameOver)
		{
			GameOver = true;
			MakeHighScore();
			Sound::stop(DMUS_SEGF_DEFAULT);
			Noise[GAMEOVER2]->play(DMUS_SEGF_SECONDARY);
		}
	}
	else
		if ((GameOver == true) & (SceneCount != 0))
		{
			GameOver = false;
			Music[track]->play(DMUS_SEGF_DEFAULT, false);
		}

	//Préparation des textes
	Screen->LoadText(Text[0], NULL, (Str)"Score: " + score, RGB(0, 0, 0), RGB(255, 255, 0));
	Screen->LoadText(Text[1], NULL, (Str)"Vie: " + life, RGB(0, 0, 0), RGB(255, 255, 0));
	Title = "Effet: ";
	switch (PlayerRacket->GetCurrentEffect())
	{
	case DESTRUCTION:
		Title.Cat("DESTRUCTION");
		break;
	case SPRING:
		Title.Cat("SPRING");
		break;
	case SPINBALL:
		Title.Cat("SPINBALL");
		break;
	case FLICKBALL:
		Title.Cat("FLICKBALL");
		break;
	case INVERSION:
		Title.Cat("INVERSION");
		break;
	case REBUILD1:
		Title.Cat("REBUILD1");
		break;
	case REBUILD2:
		Title.Cat("REBUILD2");
		break;
	case SLOWBALL:
		Title.Cat("SLOWBALL");
		break;
	case BIGBALL:
		Title.Cat("BIGBALL");
		break;
	case SONICBALL:
		Title.Cat("SONICBALL");
		break;
	case MULTIBALL:
		Title.Cat("MULTIBALL * ");
		Title.Cat(MaxShoot);
		break;
	case FREEANGLE:
		Title.Cat("FREEANGLE");
		break;
	case SKIPLEVEL:
		Title.Cat("SKIPLEVEL");
		break;
	case BONUS:
		Title.Cat("BONUS * ");
		Title.Cat(bonus);
		break;
	default:
		break;
	}
	Screen->LoadText(Text[2], NULL, Title, RGB(0, 0, 0), RGB(255, 255, 0));
	Screen->LoadText(Text[3], NULL, (Str)"Angle viseur: " + (int)-PlayerRacket->GetSight()->GetDegreeAngle(360) + (Str)"°", RGB(0, 0, 0), RGB(255, 255, 0));
	if (!fopen_s(&HighScoreFile, "highscore.dat", "rb"))
	{
		Screen->LoadText(Text[4], NULL, (Str)"Meilleur score: " + _getw(HighScoreFile), RGB(0, 0, 0), RGB(255, 255, 0));
		fclose(HighScoreFile);
	}

	//Affichage des textes
	for (i = 0; i < MAX_TEXT; i++)
		Screen->Blit(200 * i, 0, Text[i], NULL, NULL, NULL);

	Screen->FlipFlap();
}

bool ProcessBrickCollision(CardinalPoint *BrickEdge, Axe *BrickEdgeID)
{
	if (BrickEdge)
	{
		if ((Levels->GetMapData()->TileID[BrickEdgeID->y][BrickEdgeID->x][0] == 1) | (Levels->GetMapData()->TileID[BrickEdgeID->y][BrickEdgeID->x][0] == 2))
			Noise[IMPACT1]->play(DMUS_SEGF_SECONDARY);
		if ((Levels->GetMapData()->TileID[BrickEdgeID->y][BrickEdgeID->x][0] == 2) & (PlayerRacket->GetCurrentEffect() == DESTRUCTION))
		{
			Noise[IMPACT4]->play(DMUS_SEGF_SECONDARY);
			BrickRecovery[BrickEdgeID->y][BrickEdgeID->x][0] = Levels->GetMapData()->TileID[BrickEdgeID->y][BrickEdgeID->x][0];	//Conserve l'id d'origine de la brique
			Levels->GetMapData()->TileID[BrickEdgeID->y][BrickEdgeID->x][0] = 0;															//Efface la brique
		}
		if (Levels->GetMapData()->TileID[BrickEdgeID->y][BrickEdgeID->x][0] >= 3)
		{
			//La brique contient-t'elle une capsule spéciale effet?
			switch (Clock::random(0, 20))
			{
			case DESTRUCTION:
				ReleaseCapsule(Levels->GetMapData()->TileZone[BrickEdgeID->x].west, Levels->GetMapData()->TileZone[BrickEdgeID->y].north - Levels->GetScreenData()->Camera.getPosition()->y, 3, DESTRUCTION);
				break;
			case SPRING:
				ReleaseCapsule(Levels->GetMapData()->TileZone[BrickEdgeID->x].west, Levels->GetMapData()->TileZone[BrickEdgeID->y].north - Levels->GetScreenData()->Camera.getPosition()->y, 3, SPRING);
				break;
			case SPINBALL:
				ReleaseCapsule(Levels->GetMapData()->TileZone[BrickEdgeID->x].west, Levels->GetMapData()->TileZone[BrickEdgeID->y].north - Levels->GetScreenData()->Camera.getPosition()->y, 3, SPINBALL);
				break;
			case FLICKBALL:
				ReleaseCapsule(Levels->GetMapData()->TileZone[BrickEdgeID->x].west, Levels->GetMapData()->TileZone[BrickEdgeID->y].north - Levels->GetScreenData()->Camera.getPosition()->y, 3, FLICKBALL);
				break;
			case INVERSION:
				ReleaseCapsule(Levels->GetMapData()->TileZone[BrickEdgeID->x].west, Levels->GetMapData()->TileZone[BrickEdgeID->y].north - Levels->GetScreenData()->Camera.getPosition()->y, 3, INVERSION);
				break;
			case REBUILD1:
				ReleaseCapsule(Levels->GetMapData()->TileZone[BrickEdgeID->x].west, Levels->GetMapData()->TileZone[BrickEdgeID->y].north - Levels->GetScreenData()->Camera.getPosition()->y, 3, REBUILD1);
				break;
			case REBUILD2:
				ReleaseCapsule(Levels->GetMapData()->TileZone[BrickEdgeID->x].west, Levels->GetMapData()->TileZone[BrickEdgeID->y].north - Levels->GetScreenData()->Camera.getPosition()->y, 3, REBUILD2);
				break;
			case SLOWBALL:
				ReleaseCapsule(Levels->GetMapData()->TileZone[BrickEdgeID->x].west, Levels->GetMapData()->TileZone[BrickEdgeID->y].north - Levels->GetScreenData()->Camera.getPosition()->y, 3, SLOWBALL);
				break;
			case BIGBALL:
				ReleaseCapsule(Levels->GetMapData()->TileZone[BrickEdgeID->x].west, Levels->GetMapData()->TileZone[BrickEdgeID->y].north - Levels->GetScreenData()->Camera.getPosition()->y, 3, BIGBALL);
				break;
			case SONICBALL:
				ReleaseCapsule(Levels->GetMapData()->TileZone[BrickEdgeID->x].west, Levels->GetMapData()->TileZone[BrickEdgeID->y].north - Levels->GetScreenData()->Camera.getPosition()->y, 5, SONICBALL);
				break;
			case MULTIBALL:
				ReleaseCapsule(Levels->GetMapData()->TileZone[BrickEdgeID->x].west, Levels->GetMapData()->TileZone[BrickEdgeID->y].north - Levels->GetScreenData()->Camera.getPosition()->y, 5, MULTIBALL);
				break;
			case FREEANGLE:
				ReleaseCapsule(Levels->GetMapData()->TileZone[BrickEdgeID->x].west, Levels->GetMapData()->TileZone[BrickEdgeID->y].north - Levels->GetScreenData()->Camera.getPosition()->y, 5, FREEANGLE);
				break;
			case SKIPLEVEL:
				ReleaseCapsule(Levels->GetMapData()->TileZone[BrickEdgeID->x].west, Levels->GetMapData()->TileZone[BrickEdgeID->y].north - Levels->GetScreenData()->Camera.getPosition()->y, 7, SKIPLEVEL);
				break;
			case BONUS:
				ReleaseCapsule(Levels->GetMapData()->TileZone[BrickEdgeID->x].west, Levels->GetMapData()->TileZone[BrickEdgeID->y].north - Levels->GetScreenData()->Camera.getPosition()->y, 7, BONUS);
				break;
			case CANCEL:
				ReleaseCapsule(Levels->GetMapData()->TileZone[BrickEdgeID->x].west, Levels->GetMapData()->TileZone[BrickEdgeID->y].north - Levels->GetScreenData()->Camera.getPosition()->y, 5, CANCEL);
				break;
			default:
				break;
			}
			Noise[IMPACT3]->play(DMUS_SEGF_SECONDARY);
			//Augmente le score
			if (PlayerRacket->GetCurrentEffect() == BONUS)
				score += Levels->GetMapData()->TileID[BrickEdgeID->y][BrickEdgeID->x][0] * bonus;
			else
				score += Levels->GetMapData()->TileID[BrickEdgeID->y][BrickEdgeID->x][0];
			BrickRecovery[BrickEdgeID->y][BrickEdgeID->x][0] = Levels->GetMapData()->TileID[BrickEdgeID->y][BrickEdgeID->x][0];
			Levels->GetMapData()->TileID[BrickEdgeID->y][BrickEdgeID->x][0] = 0;
		}

		return true;
	}

	return false;
}

bool ProcessKbInput()
{
	int i;

	if (!keybListener.startCaptureKeys(Keyboard))
		return false;

	if (keybListener.keyPressed(SIGHT_LEFT, Config.GetKeyMap()))
	{
		PlayerRacket->GetSight()->DecreaseAngle(PI + 0.2, 2 * PI - 0.2, 180);
		if (PlayerRacket->GetCurrentEffect() == FREEANGLE)
			for (i = 0; i < Balls.GetElementCount(); i++)
				Ball::getBallElement(i, &Balls)->DecreaseAngle(0, 2 * PI, 180);
	}

	if (keybListener.keyPressed(SIGHT_RIGHT, Config.GetKeyMap()))
	{
		PlayerRacket->GetSight()->IncreaseAngle(PI + 0.2, 2 * PI - 0.2, 180);
		if (PlayerRacket->GetCurrentEffect() == FREEANGLE)
			for (i = 0; i < Balls.GetElementCount(); i++)
				Ball::getBallElement(i, &Balls)->IncreaseAngle(0, 2 * PI, 180);
	}

	if (keybListener.keyReleased(SAFEGRAVITY, Config.GetKeyMap()))
		SafeGravity = true;

	if (keybListener.keyReleased(REPLAY, Config.GetKeyMap()))
		NewGame();

	if (keybListener.keyPressed(CONFIGURATION, Config.GetKeyMap()))
	{
		ShowWindow(hWndMain, SW_MINIMIZE);
		ShellExecute(hWndMain, "open", "GameConfig.exe", NULL, NULL, SW_SHOW);
	}

	if (keybListener.keyReleased(SWITCHSCREEN, Config.GetKeyMap()))
	{
		DestroyDirectDraw();
		DestroyDirectInput();
		Screen->GetCurrentMode()->Switch();
		RestoreDirectDraw();
		InitDirectInput();
	}

	if (keybListener.keyPressed(EXIT, Config.GetKeyMap()))
		SendMessage(hWndMain, WM_QUIT, NULL, NULL);

	keybListener.captureKeys();

	return true;
}

void ProcessMouseInput()
{
	DIMOUSESTATE DIms;
	HRESULT hr;

	hr = Mouse->GetDevice()->GetDeviceState(sizeof(DIMOUSESTATE), (LPVOID)&DIms);
	if (hr != DI_OK)
		ReportDxError(S_FALSE, DIRECTINPUT, ERR_DI_GETDEVICESTATE);
	else
	{
		PlayerRacket->SetSpinTotal(DIms.lX);
		if (DIms.rgbButtons[0])
			if (life && !restart)
				if (PlayerRacket->GetCurrentEffect() == MULTIBALL)
				{
					BallWaiter.goCounter(5, 0, 1, false);
					if (!BallWaiter.isGoing() && Balls.GetElementCount() < MaxShoot)
						DrawBall();
				}
				else
					if (!Balls.GetElementCount())
						DrawBall();
		if (DIms.lX < 0)
			if (PlayerRacket->GetCurrentEffect() == INVERSION)
				PlayerRacket->SetAngle(0);
			else
				PlayerRacket->SetAngle(PI);
		if (DIms.lX > 0)
			if (PlayerRacket->GetCurrentEffect() == INVERSION)
				PlayerRacket->SetAngle(PI);
			else
				PlayerRacket->SetAngle(0);
		if (!DIms.lX)
			if (PlayerRacket->GetCurrentEffect() == SPRING)
				PlayerRacket->DecreaseMagnitude(0);
			else
				PlayerRacket->SetMagnitude(0);
		else
			if (PlayerRacket->GetCurrentEffect() == SPRING)
				PlayerRacket->IncreaseMagnitude(12);
			else
				PlayerRacket->SetMagnitude(12);
	}
}

void ReleaseCapsule(int x, int y, double speed, EFFECT effect)
{
	Capsule *pCapsule;

	if (!wait)
	{
		pCapsule = new Capsule(effect, x, y, 30, 15, PI / 2, 0.8, 0.8, "Capsule");
		pCapsule->addAni(IDB_CAPSULES, "", 16, 30, 15, RGB(255, 255, 255), 270);
		pCapsule->SetMagnitude(speed);
		Capsules.AddElement(pCapsule);
		wait = true;
		Noise[FALL]->play(DMUS_SEGF_SECONDARY);
	}
}

void RestoreDirectDraw()
{
	int i;

	Screen->SetDisplayMode();

	//Les fonds
	Screen->LoadBitmap(IDB_BACKGROUND, Background[0], 0, 270);
	Screen->LoadBitmap(IDB_BACKGROUND1, Background[1], 0, 270);
	Screen->LoadBitmap(IDB_BACKGROUND2, Background[2], 0, 270);
	Screen->LoadBitmap(IDB_BACKGROUND3, Background[3], 0, 270);
	Screen->LoadBitmap(IDB_BACKGROUND4, Background[4], 0, 270);
	Screen->LoadBitmap(IDB_BACKGROUND5, Background[5], 0, 270);
	
	//Les briques
	Levels->GetTileData()->restoreAni();

	//La raquette
	PlayerRacket->restoreAni();

	//Les capsules
	for (i = 0; i < Capsules.GetElementCount(); i++)
		Capsule::getCapsuleElement(i, &Capsules)->restoreAni();

	//Les balles
	for (i = 0; i < Balls.GetElementCount(); i++)
		Ball::getBallElement(i, &Balls)->restoreAni();
}

void SetBrickEdge(CardinalPoint *BrickEdge, Axe *BrickEdgeID, int x, int y)
{
	BrickEdge->west = Levels->GetMapData()->TileZone[x].west;
	BrickEdge->east = Levels->GetMapData()->TileZone[x].east;
	BrickEdge->north = Levels->GetMapData()->TileZone[y].north - Levels->GetScreenData()->Camera.getPosition()->y;
	BrickEdge->south = Levels->GetMapData()->TileZone[y].south - Levels->GetScreenData()->Camera.getPosition()->y;
	BrickEdgeID->x = x;
	BrickEdgeID->y = y;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow)
{
	int i;
	MSG msg;
	HMODULE ResModule;

	track = -1;

	InitDXErrorMsg();
	InitWindow(hInst, nCmdShow);
	InitDirectMusic();

	ResModule = LoadLibrary("CBRes.dll");

	Screen = new Display(hWndMain, ResModule);
	Keyboard = new Input(hWndMain, hInst);
	Mouse = new Input(hWndMain, hInst);

	for (i = 0; i < MAX_TEXT; i++)
		Text[i] = new Surface();

	PlayerRacket = new Racket(100, 20, 0.3, 0.3, "Raquette");
	Levels = new TileEngine(NULL, NULL, PI + PI / 2, PlayerRacket->GetAcceleration(), PlayerRacket->GetDeceleration());

	if (!LoadConfig())
	{
		MessageBox(hWndMain, "Impossible de charger la configuration", NULL, MB_ICONERROR);
		SendMessage(hWndMain, WM_QUIT, NULL, NULL);
		return 0;
	}

	Config.GetKeyMap()->MapName("Viser à gauche", SIGHT_LEFT);
	Config.GetKeyMap()->MapName("Viser à droite", SIGHT_RIGHT);
	Config.GetKeyMap()->MapName("Laisser tomber la balle", SAFEGRAVITY);
	Config.GetKeyMap()->MapName("Rejouer", REPLAY);
	Config.GetKeyMap()->MapName("Configuration", CONFIGURATION);
	Config.GetKeyMap()->MapName("Plein écran / Fenêtré", SWITCHSCREEN);
	Config.GetKeyMap()->MapName("Quitter", EXIT);
	Config.GetKeyMap()->OrderBy(List::ORDER_BY_NUMINDEX);

	Clock::useRandom();

	InitDirectDraw();
	InitDirectInput();
	InitObjects();
	NewGame();

	while (1)
		if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			if(!GetMessage(&msg, NULL, 0, 0 ))
				return msg.wParam;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			if (running)
			{
				if (reinit)
				{
					reinit = false;
					DestroyDirectDraw();
					DestroyDirectInput();
					RestoreDirectDraw();
					InitDirectInput();
				}
				else
				{
					PlayScene();
					ProcessMouseInput();
					ProcessKbInput();
				}
			}
		}

	return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_QUIT:
		DestroyAll();
		MakeHighScore();
		FreeLibrary(Screen->GetResInst());
		PostQuitMessage(0); //Ferme la fenêtre
		return 0;
	case WM_SETCURSOR:
		SetCursor(NULL); //Définit la valeur du curseur
		return 0;
	case WM_ACTIVATE:
		//Switché sur une autre application
		if (LOWORD(wParam) == WA_INACTIVE || (BOOL)HIWORD(wParam))
		{
			running = false;
			reinit = true;
		}
		else
			running = true;
		return 0;
	}

	if (msg == RegisterWindowMessage("GabGamesConfig"))
	{
		if (wParam == 1 && LoadConfig())
			InitObjects();
		ShowWindow(hWndMain, SW_RESTORE);
		return 0;
	}
	
	return DefWindowProc(hWnd, msg, wParam, lParam);
}
