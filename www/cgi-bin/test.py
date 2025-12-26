#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys
import cgi
from datetime import datetime

# En-têtes HTTP requis pour CGI
print("Content-Type: text/html")
print()  # Ligne vide obligatoire entre en-têtes et contenu

# Récupérer les variables d'environnement CGI
request_method = os.environ.get('REQUEST_METHOD', 'GET')
query_string = os.environ.get('QUERY_STRING', '')
content_length = os.environ.get('CONTENT_LENGTH', '0')

# HTML de la page
html = """<!DOCTYPE html>
<html lang="fr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Test CGI Python</title>
    <style>
        body {{
            font-family: Arial, sans-serif;
            max-width: 800px;
            margin: 50px auto;
            padding: 20px;
            background-color: #f5f5f5;
        }}
        .container {{
            background: white;
            padding: 30px;
            border-radius: 8px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        }}
        h1 {{
            color: #2c3e50;
            border-bottom: 3px solid #3498db;
            padding-bottom: 10px;
        }}
        .info {{
            background: #ecf0f1;
            padding: 15px;
            border-radius: 5px;
            margin: 10px 0;
        }}
        .success {{
            color: #27ae60;
            font-weight: bold;
        }}
        table {{
            width: 100%;
            border-collapse: collapse;
            margin: 20px 0;
        }}
        th, td {{
            padding: 10px;
            text-align: left;
            border-bottom: 1px solid #ddd;
        }}
        th {{
            background-color: #3498db;
            color: white;
        }}
    </style>
</head>
<body>
    <div class="container">
        <h1>✅ CGI Python fonctionne !</h1>
        <p class="success">Ce script CGI Python s'exécute correctement sur votre serveur webserv.</p>
        
        <div class="info">
            <h2>Informations de la requête</h2>
            <table>
                <tr>
                    <th>Variable</th>
                    <th>Valeur</th>
                </tr>
                <tr>
                    <td>Méthode HTTP</td>
                    <td>{method}</td>
                </tr>
                <tr>
                    <td>Query String</td>
                    <td>{query}</td>
                </tr>
                <tr>
                    <td>Content-Length</td>
                    <td>{content_len}</td>
                </tr>
                <tr>
                    <td>Date/Heure serveur</td>
                    <td>{timestamp}</td>
                </tr>
                <tr>
                    <td>Script Name</td>
                    <td>{script_name}</td>
                </tr>
                <tr>
                    <td>Server Protocol</td>
                    <td>{protocol}</td>
                </tr>
            </table>
        </div>
        
        <div class="info">
            <h2>Variables d'environnement CGI</h2>
            <table>
""".format(
    method=request_method,
    query=query_string if query_string else "(aucune)",
    content_len=content_length,
    timestamp=datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
    script_name=os.environ.get('SCRIPT_NAME', 'N/A'),
    protocol=os.environ.get('SERVER_PROTOCOL', 'N/A')
)

# Afficher toutes les variables d'environnement
env_vars = [
    'GATEWAY_INTERFACE', 'SERVER_PROTOCOL', 'SERVER_SOFTWARE',
    'REQUEST_METHOD', 'QUERY_STRING', 'CONTENT_TYPE',
    'CONTENT_LENGTH', 'SERVER_NAME', 'SERVER_PORT',
    'REMOTE_ADDR', 'SCRIPT_NAME', 'PATH_INFO'
]

for var in env_vars:
    value = os.environ.get(var, 'Non défini')
    html += f"""
                <tr>
                    <td>{var}</td>
                    <td>{value}</td>
                </tr>
"""

html += """
            </table>
        </div>
    </div>
</body>
</html>
"""

print(html)
