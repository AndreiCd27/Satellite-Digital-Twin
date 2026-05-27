# CRender (3D Renderer in C++ realizat cu OpenGL 4.3)

## Informatii generale

Dezvoltarea proiectului a inceput la finalul lunii februarie.
Acest repo a fost inital clonat din un repo ceva mai vechi de al meu https://github.com/AndreiCd27/3D-Renderer

CRender este atat un vizualizator 3D, cat si un enviroment in care utilizatorul poate crea jocuri simple 3D, avand functii implementate pentru translatii (schimbare pozitie, rotatie, dimensiuni) cat si o ierarhie de instante care este esentiala pentru scalabilitate. Proiectul contine destul de multe abstractizari pentru a face crearea de obiecte foarte usoara.

## Ce este nou?
### CRender Extensions
Extensiile functioneaza ca niste cod aditional care aduc functionalitati suplimentare aplicatiilor grafice
Momentan, am dezvoltat doar 2 extensii:
1) LightingService (Generarea de Soft-Shadows): 
	- Ofera o alternativa la render-ul clasic pentru a simula efectele difuze ale luminii
	- Are functionalitati suplimentare via Compute Shaders prin care putem voxeliza mesh-urile sau sa le descompunem intr-un set de sfere
	- Necesita in continuare update-uri pentru a elimina shadow artifacts
2) ImageService (Image Format Handling & Conversie imagine-textura)
	- Ofera abilitatea de a converti fisiere PNG in TIFF sau invers
	- Conversie simpla la texturi OpenGL, util pentru a importa texturi usor in engine
	- Operatii de downsampling / upsampling pentru imagini/texturi cu dimensiuni diferite
### Upgrade la versiunea 4.3 de OpenGL pentru Compute Shaders
Un compute shader ruleaza direct pe GPU si poate fi foarte folositor pentru orice computatie masiva care poate abuza de paralelismul de pe placa grafica
	- Compute shader de UPSAMPLING / DOWNSAMPLING pentru imagini si texturi ---> CRenderExtensions/ImageService
	- Compute shadere pentru voxelizarea geometriei, decompozitia mesh-urilor in sfere, computatii soft-shadow (vezi extensia Lighting)
### Librarii adaugate
	- stb_image (citirea si scrierea pixelilor din imaginile PNG) https://github.com/nothings/stb
	- TinyTIFF (citirea si scrierea pixelilor din fisiere TIFF) https://github.com/jkriege2/TinyTIFF
### Specific OOP
1) Factory Design Pattern (clasa ImageService)
	- Clasa ImageService se ocupa de crearea imaginilor (format PNG/TIFF), cautand in filesystem
	- Se ocupa si de salvarea imaginilor pe disk, coordonarea operatiilor de extragere pixel / resizing
	- Returneaza prin metoda Create() un unique pointer care poate fi referentiat in main
	- Imaginile se vor genera in folder-ul de Build, daca nu sunt sterse explicit inainte de inchiderea programului
	- Stergerea fisierelor inainte de incheierea programului se face cu metoda Delete()
	- Comunica cu ImageAdapter care e responsabil pentru conversii PNG <-> TIFF <-> Texture

2) Adapter Design Pattern (clasa ImageAdapter)
	- Clasa ImageAdapter se ocupa de conversiile intre formate de imagini si texturi
	- Practic, citeste pixelii folosind libraria specifica formatului, apoi:
	- Daca imaginea destinatie are dimensiuni diferite:
		- Scrie pixelii intr-o textura OpenGL cu dimensiunile imaginii sursa
		- Genereaza o textura cu dimensiunile imaginii destinatie
		- Ruleaza un Compute Shader care scrie in textura destinatie
		- Scrie pixeli din textura rezultata in imaginea destinatie
	- Daca imaginile au aceasi dimensiune, se apeleaza metoda de scriere specifica formatului imaginii destinatie

### Specific C++ modern
1) Templates
	- MultiArray.h, sistemul de container custom si cel de referinte via index-uri in container
	
	- Functii template pentru orice fel de date trimise catre GPU via Compute Shaders (SSBO)
	```
	VoxelizeMeshCompute.SetDataSSBO<GPU_Trig>(trigData, (int)trigData.size(), idxTrigBuffer);
	```
	- Functii custom pentru engine, cu argumente variabile template
	```
	Action(std::function<void(ArgsT...)> _func, ArgsT... _args) : AbstractFunc()
	```
	
2) std::function
	- Clasa Action si Request (mostenite din AbstractFunc) pentru functii personalizate in engine
	- Legarea actiunilor la clasa EngineConfig care executa seturi de actiuni la diferite momente din procesul de randare
	```
	void AddAction(const int STAGE, std::function<void(ArgsT...)> func, ArgsT... args)
	```
	
3) Multithreading
	- Folosit in accelerarea fazei de initializare, setarea datelor instantelor in container-ul custom
	- (Vezi Scene::ExecuteInstancePool() din Tile.cpp)
	```
	futures.push_back(std::async(std::launch::async, [&insArray, &matArray, i]() {
		if (!insArray[i].UpToDate) {
			insArray[i].SetDataMatrix(matArray[i]);
			insArray[i].SetDataColor(matArray[i]);
			insArray[i].UpToDate = true;
		}
	}));
	```

### Un workflow tipic ar fi urmatorul:
- Configuram engine-ul / renderer-ul
- Cream niste *Blueprint-uri* pentru viitoarele *Instante*
- Instantiem obiecte si apelam metodele pentru a pozitiona, roti, scala
(Putem eventual sa cream o ierarhie de instante folosind SetParent(), AddChild() etc.)
- Cream un game-loop, si apelam metodele engine-ului de render
- Verificam daca fereasta trebuie inchisa si oprim game-loop-ul
- Apelam OBLIGATORIU engine.EngineTerminate() dupa finalizare

### Un exemplu de program 

Exemple de cod folosind clasa Engine3D puteti gasi in directorul examples.
Cel mai explicativ este "Cube.cpp", care creaza un cub si il randeaza pe ecran.

Gasiti in directorul *src* fisierul *Main.cpp*, care e codul "default", si incearca sa arate toate functionalitatiile clasei.

Fisierul *Main.cpp* initializeaza Singleton-ul Engine3D, apoi creaza niste *Blueprint-uri* folosind metode precum CreateCube(), CreatePrism() si LoadSTLGeomFile(). Apoi creaza cateva instante ale acestora si aplica asupra lor niste translatii.

Pe ecran ar trebui sa apara un plan verde, 10 cuburi pozitionate diagonal, o prisma triunghiulara si 4 modele de oameni (modelul il gasiti in Resources). In game-loop, experimentam cu metodele de translatie, si observam ca instantele copil se translateaza odata cu parintii. 

Se afiseaza apoi in consola ierarhia instantelor prin intermediul metodelor cu prefixul 'DEBUG'.

   Rezultat
![Main.cpp](https://github.com/AndreiCd27/CRender/blob/main/Gallery/Screenshot0.png)

### Documentatie

Codul este comentat destul de des, deci puteti folosi repo-ul pentru a va crea proprile implementari. Cu toate acestea, unele clase mai necesita niste redactare. In curand voi adauga niste comentarii mai "riguroase".
Comentariile din codul sursa au fost redactate in limba engleza pentru a putea fi intelese si de vitorii citiori.

Prezentare https://canva.link/h9ep4cgp4unlef9 (partea 1)

## Instalare / Rularea proiectului

### Windows

Librariile necesare pentru proiect sunt deja incluse in directorul Libraries, iar Windows vine nativ cu OpenGL, trebuie doar sa creati build-ul ruland cmake.
Puteti sa rulati repo-ul fie in Microsoft Visual Studio, fie instalati CMake (https://cmake.org/download/) si il rulati acolo.
Executabilul cel mai probabil se va genera in directorul build care va fi creat automat de CMake.

### MacOS

Librariile necesare pentru proiect sunt deja incluse in directorul Libraries, iar MacOS vine nativ cu OpenGL, trebuie doar sa rulati cmake.
Puteti sa rulati fisierul in terminal, dar trebuie sa va asigurati ca aveti CMake pe care puteti sa il descarcati folosind un package-manager precum Homebrew (https://formulae.brew.sh/formula/cmake#default).
```
brew install cmake
```
Apoi, puteti sa rulati urmatoarele comenzii in directorul unde se afla repo-ul.
```
cd CRender/build
cmake ..
cmake --build .
```
Alternativ, instalati CMake (https://cmake.org/download/) si introduceti fisierul.
Executabilul cel mai probabil se va genera in directorul build care va fi creat automat de CMake.

### Linux

Aveti in directorul repo-ului un script shell care contine toate comenzile necesare pentru a instala OpenGL si GLFW
Aici gasiti dependintele Linux pentru OpenGL si GLFW
https://medium.com/geekculture/a-beginners-guide-to-setup-opengl-in-linux-debian-2bfe02ccd1e

Puteti instala CMake folosind package-manager-ul vostru default.

Pentru distributii Debian-based:
Instalati CMake si librariile folosite de OpenGL
```
sudo apt-get install cmake pkg-config
sudo apt-get install mesa-utils libglu1-mesa-dev freeglut3-dev mesa-common-dev
sudo apt-get install libglew-dev libglfw3-dev libglm-dev
sudo apt-get install libao-dev libmpg123-dev
```
Instalati wayland-scanner (daca nu il aveti deja)
```
sudo apt install libwayland-bin libwayland-dev
```
Instalati GLFW
```
cd /usr/local/lib/
sudo git clone https://github.com/glfw/glfw.git
cd glfw
sudo cmake .
sudo make
sudo make install
```

Apoi, puteti sa rulati urmatoarele comenzii in directorul unde se afla repo-ul.
```
cd CRender/build
cmake ..
cmake --build .
```

## Extra

Implementare LightingService (Soft-Shadows via Armonice Sferice):
(CRenderExtensions/Lighting)
	- Se foloseste de baza Armonicelor Sferice, o serie de functii pe sfera ortonormate care pot reproduce semnale complexe (in cazul acesta ocluzia geometriei), similar cu transformatele Fourier, dar pe sfera
	- Se foloseste un ordin maxim, si asta ne da o aproximare destul de buna chiar si pentru functii binare (0 - punctul e blocat in directia D, 1 - punctul primeste lumina in directia D)
	- Clasa foloseste oridnul 3, care are 16 coeficienti pe care ii stocam per voxel intr-un cubemap (textura 3D), iar apoi se reconstruieste o valoare aproximativa a ocluziei/vizibilitatii
	- Mai mult, putem simula orice directie a soarelui, pentru ca geometria este stocata direct si e independenta de directia luminii, dar se aduaga timpul de pre-fetch la initializarea engine-ului
	- Codul foloseste mult Compute Shaders, fara acestea costul computational ar fi fost prea mare
	- Detalii: https://share.google/xHvd1lybGxwgB7gO9, https://3dvar.com/Green2003Spherical.pdf

## Arhitectura

### Diagrama Claselor
![Diagrama Claselor (Fara detalii)](https://github.com/AndreiCd27/CRender/blob/main/Gallery/AllClassesDiagram.png)
### Diagrama Claselor (Detaliat) - 1
![Diagrama Claselor (Top Level)](https://github.com/AndreiCd27/CRender/blob/main/Gallery/ClassDiagramTopLevel.png)
### Diagrama Claselor (Detaliat) - 2
![Diagrama Claselor (Low Level)](https://github.com/AndreiCd27/CRender/blob/main/Gallery/ClassDiagramLowLevel.png)

### Mostenirea
Intr-un motor grafic de obicei se prefera compozitia mai mult decat mostenirea, si de aceea am ales sa merg pe aceasta cale.
In opinia mea, este mult mai extensibil un sistem compozit in acest context.
De exemplu, puteam sa fac clasa Engine3D sa mosteneasca clasa Scene, dar apareau dificultati in momentul in care doream sa putem sa interschimbam o scena cu alta.
In sistemul actual, problema se rezolva prin implementarea unui std::vector<Scene*>, si un parametru Scene* scene la toate functiile de draw, cu actualizarea obiectelor OpenGL prin glBufferSubData() sau glBufferData(). Alternativ, stocam VAO, VBO, EBO, instanceVBO in clasa Scene.

Cu toate acestea, am observat ca mostenirea poate fi folositoare la obiecte "mici" si similare, dar cu behaviour diferit.
Asadar, pe langa relatiile de mostenire Instance ---|> Transform ---|> Entity, voi mai enumera inca 3 clase care pot fi adaugate pe viitor care se afla in relatii de mostenire.
(De mentionat si ca avem clasele ShaderException, SceneException, InstanceException, ArrayOrganizerException si BlueprintException care mostenesc std::runtime_error, deci teoretic avem *7* clase cu relatii de mostenire)

1) Clasa UI_Label ---|> Transform
   - Mosteneste de la Transform; interpretam axa Z ca fiind fata-spate, astfel determinam ce elemente UI trebuie puse in fata sau in spatele altora
   - Update() special care ne asigura ca UI-ul se deseneaza *ultimul*, si seteaza toate atributele speciale pentru UI
   - Metode unice clasei: SetText(text), SetBorderSize(size_px), etc.

2) Clasa UI_Interactive ---|> Transform
   - Metode unice clasei: Input(key), InputMouse(clickType), InputHover() etc. | toate returneaza *true* daca la frame-ul curent input-ul s-a "declansat" si utilizatorul creaza o functie in Main.cpp care interpreteaza input-ul la frame-ul curent
   - Alta metoda care poate fi extinsa: SetImage()
   - Update() overwritten pentru a implementa metodele speciale

3) Clasa UI_Button ---|> UI_Interactive, UI_Label *(mostenire de tip diamant)*
   - Update() overwritten care asigura concordanta dintre date (de exemplu: sa nu desenam imaginea peste text, dar nici backgroundColor peste imagine)
