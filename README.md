# Projeto de Programação Multithread em C

## Descrição do Projeto

Este projeto consiste em um programa desenvolvido em C que realiza operações com matrizes utilizando múltiplos threads de processamento. As principais funcionalidades incluem a leitura de matrizes a partir de arquivos, a realização de operações de soma e multiplicação entre as matrizes, a gravação dos resultados em novos arquivos, e a redução por soma dos elementos da matriz final.
Requisitos

## Para compilar e executar este programa, é necessário ter instalado:
-  Um compilador de C (como GCC)
- Biblioteca pthread (para suporte a threads em POSIX)

## Compilação:
```bash
gcc -o programa programa.c -lpthread
```
Alternativamente utilize o comando "make" no terminal.

## Execução:

O programa deve ser executado com a seguinte sintaxe de linha de comando:
```bash
./programa T n arqA.dat arqB.dat arqC.dat arqD.dat arqE.dat
```

Onde:
- T é o número de threads de processamento.
- n é o tamanho das matrizes (n x n).
- arqA.dat, arqB.dat, arqC.dat são os arquivos contendo as matrizes de entrada A, B e C, respectivamente.
- arqD.dat e arqE.dat são os arquivos onde serão salvos os resultados das operações com as matrizes D e E, respectivamente.

## Exemplo de Execução:
Para executar o programa com 4 threads de processamento e matrizes de tamanho 100x100:

```sh
./programa 4 100 arqA.dat arqB.dat arqC.dat arqD.dat arqE.dat
```
