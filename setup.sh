#!/bin/bash
mkdir -p $HOME/projects/workspace

PWD=`pwd`
for path in .bashrc .gitconfig .tmux.conf .vimrc bashrc
do
    ln -sf $PWD/$path $HOME/$path
done

echo "Run ssh-keygen if you would like to upload ~/.ssh/id_rsa.pub to github"
echo "Close windows firewall public network part if you've set proxy and check proxy port in bashrc/functions"
echo "Run git clone https://github.com/VundleVim/Vundle.vim.git ~/.vim/bundle/Vundle.vim to install vim plugin bundles and then run PluginInstall inside vim"
echo "You may have to run ./install.py -h under ~/.vim/bundle/YouCompleteMe"
