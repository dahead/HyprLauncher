Bash script, that uses alacritty, to display all applications found on the current linux system.

The bash script calls the "Updater" C application which:
* creates a app index file in ~/.appindex
* which gets updates every 24h
* and contains all commands from compgen -c
* and also all applications from ~/.local/share/applications
