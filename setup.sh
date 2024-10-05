#!/bin/bash
mkdir -p $HOME/projects/workspace
BACK_UP=$HOME/bashrc.bak/$(date +%Y-%m-%d_%H-%M-%S)
mkdir -p $BACK_UP
PWD=`pwd`

for path in .bashrc .git-prompt.sh .gitconfig .tmux.conf .vimrc bashrc
do
    # -L to copy real data instead of symlink
    cp -rL $HOME/$path $BACK_UP/$path
    rm -rf $HOME/$path
    ln -sf $PWD/$path $HOME/$path
done

echo "Run ssh-keygen if you would like to upload ~/.ssh/id_rsa.pub to github"
echo "Close windows firewall public network part if you've set proxy and check proxy port in bashrc/functions"
echo "Run git clone https://github.com/VundleVim/Vundle.vim.git ~/.vim/bundle/Vundle.vim to install vim plugin bundles and then run PluginInstall inside vim"
echo "You may have to run ./install.py -h under ~/.vim/bundle/YouCompleteMe"

echo "Previous bashrc settings saved to $BACK_UP"
