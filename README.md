# Klima Simulator

Jednostavan 2D OpenGL demo klime sa sledećim ponašanjem:
- Levi klik na lampicu pali/gasi klimu; vent se animira, ekrani zasvetle, i status ikona (vatra/pahulja/kukica) pokazuje odnos željene i trenutne temperature.
- Strelice na tastaturi ili klik na gornju/donju polovinu streličnog panela menjaju željenu temperaturu u opsegu -10°C do 40°C.
- Trenutna temperatura se polako približava željenoj dok je klima upaljena.
- Lavor se puni vodom svake sekunde dok klima radi; razmaknica (`Space`) ga prazni. Kada se napuni, klima se gasi i zaključava dok se ne isprazni.
- Kursor se učitava iz `Resources/cursor.png` ako postoji, u suprotnom se koristi proceduralni.

## Pokretanje
Projekat je Visual Studio C++ (OpenGL/GLFW/GLEW). Otvoriti `ac-simulator.sln`, build, i pokrenuti (Debug/Release, x64). Fullscreen i promena veličine su podržani; scena se automatski centrira.
