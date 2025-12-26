# Webserv - Serveur HTTP en C++98

Serveur HTTP conforme au sujet 42, implémentant les fonctionnalités minimales requises.

## Fonctionnalités implémentées ✅

### Core Features
- ✅ Serveur HTTP non-bloquant avec `epoll`
- ✅ Support de plusieurs ports (multi-serveurs)
- ✅ Méthodes HTTP : GET, POST, DELETE
- ✅ Serveur de fichiers statiques
- ✅ Upload de fichiers
- ✅ Configuration flexible via fichier

### CGI
- ✅ Exécution de scripts CGI (Python, PHP, etc.)
- ✅ Variables d'environnement CGI complètes
- ✅ Gestion des timeouts (30 secondes)
- ✅ Support POST et GET pour CGI
- ✅ Parsing des réponses CGI

### Configuration
- ✅ Directive `listen` (interface:port)
- ✅ Directive `client_max_body_size` avec validation
- ✅ Pages d'erreur personnalisées (`error_page`)
- ✅ Locations avec routage
- ✅ Méthodes autorisées par location (`allow_methods`)
- ✅ Autoindex (listage de répertoires)
- ✅ Fichier index par défaut
- ✅ Chemin d'upload personnalisé
- ✅ Redirections HTTP (301, 302, etc.)
- ✅ Configuration CGI (interpréteur + extension)

### Gestion des erreurs
- ✅ Pages d'erreur par défaut (400, 403, 404, 405, 413, 500, 504)
- ✅ Codes de statut HTTP corrects
- ✅ Validation de la taille du body (413)
- ✅ Timeout des requêtes clients

## Compilation

```bash
make
```

Compile avec les flags : `-Wall -Wextra -Werror -std=c++98`

## Utilisation

```bash
./webserv [fichier_configuration]
```

## Fichier de configuration

Voir `example.conf` pour un exemple complet.

## Tests CGI

Script de test fourni : `www/cgi-bin/test.py`

## Conformité

Implémente les fonctionnalités de base HTTP/1.0 selon le sujet 42.
