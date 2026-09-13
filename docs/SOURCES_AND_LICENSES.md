# Sources et licences

État vérifié le 12 septembre 2026. Ce fichier distingue les composants ; aucune licence globale libre n’est revendiquée pour le projet.

## Données CThead

- **Source : University of North Carolina**, données fournies par le **North Carolina Memorial Hospital**, acquises sur un scanner General Electric.
- Distributeur : [Stanford Volume Data Archive](https://graphics.stanford.edu/data/voldata/).
- [Archive originale](https://graphics.stanford.edu/data/voldata/CThead.tar.gz), [notice](https://graphics.stanford.edu/data/voldata/CThead.info), [annonce UNC](https://graphics.stanford.edu/data/voldata/Announcement).
- Stanford déclare ces données dans le **domaine public**. UNC et Stanford demandent que leur redistribution conserve l’attribution « University of North Carolina », la notice et l’annonce. Ces documents sont inclus dans `data/cthead_original/`.
- La collection est attribuée à Marc Levoy (1987–1989), assemblée par Graham Gash (UNC), puis reformatée par Bill Lorensen (General Electric).

Les 113 fichiers `data/cthead/CThead.1` à `CThead.113` sont extraits sans modification de l’archive. `SHA256.json` contient leurs empreintes. Aucune coupe n’est supprimée ou rééchantillonnée sur disque. L’ancienne adaptation à 94 coupes a été retirée.

La page Stanford et `CThead.info` décrivent la version distribuée en **big-endian**. L’annonce UNC décrit un format VAX antérieur ; pour le lecteur présent, c’est bien le format Stanford qui fait foi.

CThead est une acquisition CT scalaire : il n’y a dans les fichiers ni masque du cerveau, ni contour des méninges, ni carte des artères, ni métadonnées DICOM patient. Le cerveau affiché par le mode 8 est donc un proxy visuel fondé sur une fonction de transfert d’intensité. Les méninges et les artères ne peuvent pas être filtrées de manière anatomiquement fiable avec ce jeu seul. Une visualisation des artères demanderait au minimum un angio-CT avec produit de contraste et, idéalement, une segmentation validée.

## Code de ce projet

Le projet fourni par l’utilisateur était attribué à **Michel Grave**, travail pédagogique de rendu volumique, avec la date février 2004 dans les fichiers et une mention d’usage éducatif dans le README. Les références locales se trouvent dans `/Users/ggallon/Desktop/Grave`, notamment `OpenGL/newtete.tar` et les travaux IUP3 de 2004. Le document `OpenGL/TTT/alire` est daté décembre 1999.

**Aucune licence explicite de redistribution du code d’origine n’a été identifiée dans les fichiers consultés.** La mention « éducatif » n’établit pas à elle seule une licence open source. Les auteurs et droits éventuels des adaptations intermédiaires restent à clarifier avant de présenter l’ensemble comme librement redistribuable. Il n’est pas attribué de licence MIT, GPL ou autre par défaut.

La version actuelle est une adaptation : consolidation GLFW, chargement CThead complet, géométrie physique, composition volumique, coupes fenêtrées, affichage et tests. Elle n’est pas présentée comme le code original de Michel Grave. L’attribution historique est conservée dans les sources.

Les anciens programmes, binaires et en-têtes `CTHEAD.HDR` incomplets ont été retirés du projet actif. Leur présence dans le dossier d’origine reste inchangée. Une sauvegarde de l’état avant nettoyage a été créée dans `/tmp/skull-before-cleanup-20260912.tar.gz` (stockage temporaire, pas une archive durable).

## Dépendances

| Composant | Utilisation | Licence / provenance |
| --- | --- | --- |
| GLFW | Fenêtre, clavier, souris, contexte OpenGL | [zlib/libpng](https://www.glfw.org/license.html), notice dans `GLFW-LICENSE.md` |
| OpenGL système | Affichage d’une texture via OpenGL 2.1 | Fourni par le système et le pilote ; conditions de leur fournisseur, pas de licence globale déduite de l’API |
| Bibliothèque standard C++ | Calcul, fichiers, conteneurs | Implémentation fournie par le compilateur ; conditions de cette implémentation |
| CMake, Make, compilateur | Outils de construction | Outils externes, non incorporés comme sources au projet |

GLFW est lié depuis l’installation système. Les sources de GLFW ne sont pas copiées dans ce dépôt.
