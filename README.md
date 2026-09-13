# Skull — visualisation CThead avec GLFW

Un seul projet C++ pour explorer le scanner CThead : surfaces osseuses et tissus, transparence volumique et coupes. Le jeu complet de **113 coupes de 256 × 256 pixels** est inclus avec ses notices.

## Démarrer

Sur macOS, avec les outils de compilation Xcode et GLFW installés (`brew install glfw`) :

```sh
./launch.sh
```

Ou `make && ./bin/skull`. Le lanceur fonctionne aussi depuis un autre dossier. L’exécutable utilise par défaut le chemin des données du projet au moment de la compilation ; après déplacement, recompiler ou passer `--data /chemin/vers/cthead`.

Sous Linux : GLFW, OpenGL, pkg-config et un compilateur C++11 sont requis. Cette plateforme n’a pas été testée dans cette session.

Construction alternative :

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
./build/skull
```

## Commandes

| Commande | Action |
| --- | --- |
| `1` | Surface des tissus |
| `2` / `3` | Comparaison tissus/os sur les deux moitiés de l’image |
| `4` | Surface osseuse (au démarrage) |
| `5` | Transparence volumique, composition des voxels de l’avant vers l’arrière |
| `6` | Coupe en niveaux de gris |
| `7` | Éclaté de cinq coupes réelles CThead |
| `8` | Peau translucide et cerveau visible par transfert d’intensité |
| `+` / `-` | Augmenter / diminuer l’opacité en mode 5 ; zéro rend tout transparent |
| Molette, `[` / `]`, Page bas / Page haut | Déplacer la coupe ; bornes automatiquement limitées |
| Début / Fin | Première / dernière position de coupe |
| `C` | Activer le retrait de la partie devant la coupe, en modes 1 à 5 |
| Flèches | Tourner progressivement l’image et l’éclaté, par pas de 8° |
| `X` / `Y` / `Z` | Revenir aux vues axiale / coronale / sagittale du jeu, coupe centrée et découpe désactivée |
| `o` / `O` | Diminuer / augmenter le seuil osseux |
| `t` / `T` | Diminuer / augmenter le seuil des tissus |
| `w` / `W` | Réduire / élargir la fenêtre d’intensité des coupes |
| `l` / `L` | Diminuer / augmenter le niveau central des coupes |
| `R` | Réinitialiser la vue et les réglages |
| Échap | Quitter |

Les réglages figurent dans le titre de la fenêtre. Les touches majuscules utilisent Maj. La molette et Page haut/bas évitent la difficulté de saisir les crochets sur un clavier AZERTY. Les touches 1–6 utilisent les positions numériques GLFW.

En mode `7`, l’éclaté est construit à partir de cinq coupes réellement chargées depuis `data/cthead/`. Elles sont espacées graphiquement sur une diagonale et tournent ensemble avec les flèches. La touche `C` active la coupe dans les modes volumique et affiche la légende ; `H` masque ou réaffiche la légende.

Le mode `8` applique un transfert d’intensité dédié : l’enveloppe osseuse et les tissus superficiels sont rendus translucides, puis les intensités internes sont accumulées pour faire apparaître une couche cérébrale estimée. Ajuster `+`/`-` modifie l’opacité ; `C` retire la partie située devant la coupe. Cette visualisation est pédagogique et ne constitue pas une segmentation anatomique.

La légende indique aussi les couches indisponibles. Le jeu CThead ne contient pas les données des méninges ni des artères. Un simple seuil ne permettrait pas de distinguer correctement une artère de l’os ou d’un autre tissu dense ; pour les afficher et les filtrer, il faut un angio-CT avec contraste et une segmentation dédiée.

Pour explorer l’intérieur : passer en `5`, activer `C`, puis déplacer le plan avec la molette. Passer en `6` montre le même plan, dans le même repère. Le changement de mode préserve la position de la coupe. En mode coupe, l’image reste opaque pour faciliter sa lecture ; l’opacité globale agit sur le mode volumique.

## Alignement et rendu

Les fichiers originaux sont lus comme des entiers 16 bits big-endian, sans masquage d’intensité. Les coordonnées fichier `(colonne, ligne, coupe)` deviennent en mémoire `(colonne, 112-coupe, 255-ligne)` pour une présentation coronale debout. Les pas associés deviennent `(1, 2, 1)` : ils correspondent au rapport original **1:1:2**, dans le nouvel ordre des axes. Les rotations permutent ensemble voxels, dimensions et pas.

La taille affichée suit l’étendue physique entre centres de voxels `(dimension−1) × pas`. La fenêtre conserve ce rapport, centre le volume et utilise la résolution réelle du framebuffer, y compris Retina. Le volume et les coupes partagent la même interpolation bilinéaire et le même sens vertical.

Les labels de vues se rapportent à l’organisation du jeu ; les fichiers n’établissent pas de repère patient DICOM fiable, de latéralité gauche/droite, ni d’échelle absolue en millimètres. Il n’y a qu’un volume : aucun recalage entre patients ou acquisitions n’est effectué. Les intensités sont celles du fichier, sans conversion revendiquée en unités Hounsfield.

La transparence accumule couleurs et opacités dans l’ordre de profondeur, avec correction de l’opacité selon le pas du rayon et arrêt anticipé après saturation. La coupe 3D retire les voxels devant le plan ; le mode 6 montre une section fenêtrée en niveaux de gris. Ces opérations sont des outils pédagogiques, pas un logiciel de diagnostic.

Le calcul CPU est effectué seulement lors d’un changement de réglage. L’image est interpolée à l’affichage. Limites : rotations par quarts de tour, résolution de calcul limitée à 384 pixels sur le grand axe, OpenGL 2.1 déprécié sur macOS, réglages de transfert simplifiés. Le plan conserve sa position relative en profondeur après rotation, pas un point anatomique fixe.

## Organisation

- `src/main.cpp` : application GLFW, commandes et affichage.
- `src/volume_renderer.cpp`, `include/volume_renderer.h` : chargement, géométrie, rendu et coupes.
- `data/cthead/` : 113 coupes originales.
- `data/cthead_original/` : archive source, notices et empreintes SHA-256.
- `tests/renderer_test.cpp` : régressions du moteur, sans fenêtre.
- `docs/SOURCES_AND_LICENSES.md` : provenance, licences et points non établis.

Les versions GLUT, les anciens binaires et la série convertie à 94 coupes ont été retirés.

## Vérifications

```sh
make test
mkdir -p /tmp/skull-images
./bin/skull --snapshots /tmp/skull-images
```

La seconde commande exporte les six modes au format PPM sans ouvrir de fenêtre. Le répertoire de destination doit exister. `./bin/skull --help` affiche les options.

Voir [sources et licences](docs/SOURCES_AND_LICENSES.md). Les données proviennent de l’**University of North Carolina**, via l’[archive Stanford](https://graphics.stanford.edu/data/voldata/), avec le concours du **North Carolina Memorial Hospital**. L’origine pédagogique du code est attribuée à **Michel Grave** ; sa licence de redistribution reste à clarifier.
