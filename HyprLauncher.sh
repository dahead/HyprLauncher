#!/bin/bash
# Uses alacritty, fzf, compgen and hyprctl to display a list of all applications (compgen)
# and let the user choose (alacritty) which one to execute.
exec alacritty --title "HyprLauncher" -e bash -c "compgen -c | fzf | xargs hyprctl dispatch exec" 
