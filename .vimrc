set nocompatible              " be iMproved, required
filetype off                  " required

" set the runtime path to include Vundle and initialize
set rtp+=~/.vim/bundle/Vundle.vim
call vundle#begin()
" alternatively, pass a path where Vundle should install plugins
"call vundle#begin('~/some/path/here')

" let Vundle manage Vundle, required
Plugin 'VundleVim/Vundle.vim'
Plugin 'Valloric/YouCompleteMe'
Plugin 'preservim/nerdtree'
Plugin 'kien/ctrlp.vim'
Plugin 'vim-scripts/a.vim'
Plugin 'ayu-theme/ayu-vim'
Plugin 'google/vim-maktaba'
Plugin 'bazelbuild/vim-bazel'
Plugin 'durandj/bazel.vim'
Plugin 'vim-scripts/grep.vim'

" All of your Plugins must be added before the following line
call vundle#end()            " required
filetype plugin indent on    " required
" To ignore plugin indent changes, instead use:
"filetype plugin on
"
" Brief help
" :PluginList       - lists configured plugins
" :PluginInstall    - installs plugins; append `!` to update or just :PluginUpdate
" :PluginSearch foo - searches for foo; append `!` to refresh local cache
" :PluginClean      - confirms removal of unused plugins; append `!` to auto-approve removal
"
" see :h vundle for more details or wiki for FAQ
" Put your non-Plugin stuff after this line
"
"
set number
set mouse=a


" Env settings for you complete me
let g:ycm_python_binary_path='/usr/bin/python'
let g:ycm_server_python_interpreter='/usr/bin/python'
let g:ycm_clangd_binary_path='/usr/bin/clangd'
let g:ycm_confirm_extra_conf=0

" Key bindings for you complete me
nmap K	:YcmCompleter GetDoc<CR>
nmap g  :YcmCompleter GoTo<CR>                                        
nmap gd :YcmCompleter GoToDefinition<CR>
nmap gy :YcmCompleter GoToDeclaration<CR>
nmap gr :YcmCompleter GoToReferences<CR>
nmap <F2> viwy:YcmCompleter RefactorRename<space><c-r>0
nmap <leader>f :YcmCompleter FixIt<CR>
nmap F	:YcmCompleter Format<CR>

" c/c++ language server sepcific config
autocmd FileType c,cpp vmap <buffer> <slient> = :YcmCompleter Format<CR>
autocmd FileType c,cpp vmap <buffer> :YcmCompleter GetDocImprecise<CR>
autocmd FileType c,cpp vmap <buffer> gd :YcmCompleter GoToImprecise<CR>

" Key bindings for NERDTree
nmap <leader>n :NERDTreeFocus<CR>
nmap <C-n> :NERDTree<CR>
nmap <C-t> :NERDTreeToggle<CR>
nmap <C-f> :NERDTreeFind<CR>

" Set for ayu-vim
set termguicolors
let ayucolor="light"
let ayucolor="mirage"
let ayucolor="dark"
colorscheme ayu

" Set for grep.vim
nmap <leader><leader>g :Grep<CR>
