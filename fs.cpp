/**
 * Implemente aqui as funções dos sistema de arquivos que simula EXT3
 */

/**
 * Aluno: Raphael Abreu Farias de Jesus
 * Matrícula: 20150460
 * Trabalho: Desenvolver um sistema de arquivos que simula padrão EXT3. O sistema deve ser desenvolvido em C ou C++ utilizando o compilador GNU GCC e chamadas de sistemas do padrão POSIX.
 * Disciplina: Arquitetura de Sistemas Operacionais - DEC7556
 * Turma: 07655
 * Data de entrega: 03/07/2022
 */

#include "fs.h"
#include "math.h"
#include <string.h>
#include <bitset>
#include <cassert>
#include <iostream>
#include <vector>
#include <sstream>

void initFs(std::string fsFileName, int blockSize, int numBlocks, int numInodes)
{
    FILE *file = fopen(fsFileName.c_str(), "w+");

    size_t sizeOfChar = sizeof(char);        // Size of a char
    fwrite(&blockSize, sizeOfChar, 1, file); // Byte 1 - Size of the block
    fwrite(&numBlocks, sizeOfChar, 1, file); // Byte 2 - Number of blocks
    fwrite(&numInodes, sizeOfChar, 1, file); // Byte 3 - Number of Inodes

    char bitMap = 0x01;                   // First block is used by root
    fwrite(&bitMap, sizeOfChar, 1, file); // Byte 4 - Define block 0 as used because of root (first bit of Bitmap)

    char sizeOfBitmap = (numBlocks - 1) / 8;                 // Size of the Bitmap (-1 because of root)
    int sizeOfInodeVector = (numInodes - 1) * sizeof(INODE); // Size of the Inode Vector (-1 because of root)
    char empty[sizeOfInodeVector];                           // Char vector filled with 0 to set 0 values on fwrite

    for (int i = 0; i < sizeof(empty); ++i)
    {
        empty[i] = 0;
    }

    fwrite(&empty, sizeOfChar, sizeOfBitmap, file); // Byte 5 to Byte(5 + bitmap size -1) - Fill the rest of bitmap with 0 (all bits of bitmap)

    INODE root = {
        // First inode is the directory "/"
        0x01,               // 0x01 if used, 0x00 if free
        0x01,               // 0x01 if directory, 0x00 if file
        "/",                // Name of file/directory
        0x00,               // Size of file/directory in bytes
        {0x00, 0x00, 0x00}, // Direct blocks
        {0x00, 0x00, 0x00}, // Indirect blocks
        {0x00, 0x00, 0x00}, // Double indirect blocks
    };

    fwrite(&root, sizeOfChar, sizeof(INODE), file);      // Add root inode in file
    fwrite(&empty, sizeOfChar, sizeOfInodeVector, file); // Fill the rest of inodes with 0
    fwrite(&empty, sizeOfChar, 1, file);                 // Index of root

    int sizeOfBlockVector = numBlocks * blockSize;       // Size of the Block Vector
    fwrite(&empty, sizeOfChar, sizeOfBlockVector, file); // Fill the block vector wuth 0 (because it's empty)

    fclose(file);
}

void addFile(std::string fsFileName, std::string filePath, std::string fileContent)
{
    FILE *file = fopen(fsFileName.c_str(), "r+");

    char blockSize, numBlocks, numInodes, tempValues[3];
    size_t sizeOfChar = sizeof(char); // Size of a char

    fread(&tempValues, sizeOfChar, 3, file); // Read block size, number of blocks and number of inodes
    blockSize = tempValues[0];
    numBlocks = tempValues[1];
    numInodes = tempValues[2];

    char sizeOfBitmap = ceil(numBlocks / 8.0);         // Size of the Bitmap
    int sizeOfInodeVector = numInodes * sizeof(INODE); // Size of the Inode Vector
    int sizeOfBlockVector = numBlocks * blockSize;     // Size of the Block Vector

    // Find first empty inode
    INODE inode;
    int freeInodeIndex; // Index of a free inode
    int goTo = 3 + sizeOfBitmap;
    fseek(file, goTo, SEEK_SET);
    for (int i = 0; i < numInodes; i++)
    {
        fread(&inode, sizeof(INODE), 1, file); // Read inode to find an empty one

        if (inode.IS_USED == 0)
        {
            freeInodeIndex = i;
            break;
        }
    }

    int directoriesCounter = 0, slashIndex = 0; // Number of directories
    char directoryName[10], pathName[10];       // Name of the path and directory

    for (size_t i = 0; i < strlen(filePath.c_str()); i++)
    {
        if (filePath.at(i) == '/')
        {
            directoriesCounter++;
            slashIndex = i;
        }
    }

    // Set path and directory names
    bool blockDirectoryName = false, blockPathName = false;
    for (size_t i = 0; i < sizeof(pathName); i++)
    {
        if (directoriesCounter == 1)
        {
            if ((i + 1) < strlen(filePath.c_str()))
                pathName[i] = filePath.at(i + 1);
            else
                pathName[i] = 0;

            if (filePath.at(i) == '/')
                directoryName[i] = '/';
            else
                directoryName[i] = 0;
        }

        else
        {
            if ((i + 1) < strlen(filePath.c_str()) && filePath.at(i + 1) != '/' && !blockDirectoryName)
                directoryName[i] = filePath.at(i + 1);
            else
            {
                directoryName[i] = 0;
                blockDirectoryName = true;
            }

            if ((slashIndex + 1) < strlen(filePath.c_str()) && filePath.at(slashIndex + 1) != '/' && !blockPathName)
                pathName[i] = filePath.at(slashIndex + 1);
            else
            {
                pathName[i] = 0;
                blockPathName = true;
            }
            slashIndex++;
        }
    }

    char blockValues[sizeOfBlockVector];
    goTo = 3 + sizeOfBitmap + sizeOfInodeVector + 1;
    fseek(file, goTo, SEEK_SET);
    fread(&blockValues, sizeOfChar, sizeof(blockValues), file); // Read the block vector value

    // Size of repozitory files
    if (directoryName[0] != '/')
    {
        goTo = 3 + sizeOfBitmap;
        fseek(file, goTo, SEEK_SET);
        for (int i = 0; i < numInodes; i++)
        {
            fread(&inode, sizeof(INODE), 1, file);
            if (!strcmp(inode.NAME, directoryName))
            {
                char sizeOfDirectory = 0;
                goTo = 3 + sizeOfBitmap + i * sizeof(INODE) + 12;
                fseek(file, goTo, SEEK_SET);
                fread(&sizeOfDirectory, sizeOfChar, 1, file); // Read the size of directory
                fseek(file, -1, SEEK_CUR);
                sizeOfDirectory++;
                fwrite(&sizeOfDirectory, sizeOfChar, 1, file); // Increase the size of directory in 1

                goTo = 3 + sizeOfBitmap + sizeOfInodeVector + 1 + blockSize * inode.DIRECT_BLOCKS[0];
                fseek(file, goTo, SEEK_SET);
                fwrite(&freeInodeIndex, sizeOfChar, 1, file); // Save Inode index on block

                goTo = 3 + sizeOfBitmap + i * sizeof(INODE) + sizeof(INODE);
                fseek(file, goTo, SEEK_SET);
            }
        }
    }

    char freeBlockIndex = 0; // Index of a free block
    goTo = 3 + sizeOfBitmap;
    fseek(file, goTo, SEEK_SET);
    for (int i = 0; i < numInodes; i++)
    {
        fread(&inode, sizeof(INODE), 1, file); // Read inode to count blocks

        for (size_t j = 0; j < sizeof(inode.DIRECT_BLOCKS); j++)
        {
            if (inode.DIRECT_BLOCKS[j] > freeBlockIndex)
            {
                freeBlockIndex = inode.DIRECT_BLOCKS[j];
            }
        }
    }
    freeBlockIndex++;

    char setFileInodeUsed[2] = {0x01, 0x00};                  // Set the inode of file as file and used
    goTo = 3 + sizeOfBitmap + sizeof(INODE) * freeInodeIndex; // Reference to use with fseek

    fseek(file, goTo, SEEK_SET);
    fwrite(&setFileInodeUsed, sizeOfChar, 2, file); // Set as used and as file
    fwrite(&pathName, sizeOfChar, 10, file);        // Add the file name

    int sizeOfContent = strlen(fileContent.c_str());
    fwrite(&sizeOfContent, sizeOfChar, 1, file); // Add the size of the content in the file

    char numberOfBlocks = ceil(sizeOfContent / (double)blockSize);

    for (size_t i = 0; i < numberOfBlocks; i++)
    {
        fwrite(&freeBlockIndex, sizeOfChar, 1, file); // Add blocks id in file starting in 1;
        freeBlockIndex++;
    }

    char contentOfFile[strlen(fileContent.c_str())]; // The content to be saved on file
    for (size_t i = 0; i < strlen(fileContent.c_str()); i++)
    {
        contentOfFile[i] = fileContent.at(i);
    }

    // QUICK FIX to save index and content on blocks
    if (directoryName[0] != '/')
    {
        char directoryBlock[] = {1, 2};
        goTo = 3 + sizeOfBitmap + sizeOfInodeVector + 1;
        fseek(file, goTo, SEEK_SET);
        fwrite(&directoryBlock, sizeOfChar, 2, file); // Add 01 and 02 because of the blocks used by path

        goTo = 3 + sizeOfBitmap + sizeOfInodeVector + 1 + blockSize * (freeBlockIndex - numberOfBlocks);
        fseek(file, goTo, SEEK_SET);
        fwrite(&contentOfFile, sizeOfChar, strlen(fileContent.c_str()), file); // Add the file content in the blocks;
    }

    else
    {
        char directoryBlock[] = {1, 0};
        goTo = 3 + sizeOfBitmap + sizeOfInodeVector + 1;
        fseek(file, goTo, SEEK_SET);
        fwrite(&directoryBlock, sizeOfChar, 2, file);                          // Add 01 and 00 because of block used by directory
        fwrite(&contentOfFile, sizeOfChar, strlen(fileContent.c_str()), file); // Add the file content in the blocks;
    }

    // QUICK FIX to the number of children
    int numberOfChildren = 1;
    if (directoryName[0] != '/')
        numberOfChildren = 2;

    goTo = 3 + sizeOfBitmap + 12;
    fseek(file, goTo, SEEK_SET);
    fwrite(&numberOfChildren, sizeOfChar, 1, file); // Add how many children the directory has

    char numberOfBlocksUsed = 0; // Index of a free block
    goTo = 3 + sizeOfBitmap;
    fseek(file, goTo, SEEK_SET);
    for (int i = 0; i < numInodes; i++)
    {
        fread(&inode, sizeof(INODE), 1, file); // Read inode to count blocks

        for (size_t j = 0; j < sizeof(inode.DIRECT_BLOCKS); j++)
        {
            if (inode.DIRECT_BLOCKS[j] > numberOfBlocksUsed)
            {
                numberOfBlocksUsed = inode.DIRECT_BLOCKS[j];
            }
        }
    }

    int counterOfFilled = 0;
    int valueToHex = 0; // Value of filled blocks in binary to dacimal
    for (size_t i = 0; i <= numberOfBlocksUsed; i++)
    {

        valueToHex += pow(2, counterOfFilled);
        counterOfFilled++;
    }
    fseek(file, 3, SEEK_SET);
    fwrite(&valueToHex, sizeOfChar, 1, file); // Add the number of block filled

    fclose(file); // Close file
}

void addDir(std::string fsFileName, std::string dirPath)
{
    FILE *file = fopen(fsFileName.c_str(), "r+");

    char tempValues[3];
    int blockSize, numBlocks, numInodes;
    size_t sizeOfChar = sizeof(char); // Size of a char

    fread(&tempValues, sizeOfChar, 3, file); // Read block size, number, blocks and number of inodes and bitmap
    blockSize = tempValues[0];
    numBlocks = tempValues[1];
    numInodes = tempValues[2];

    int sizeOfInodeVector = numInodes * sizeof(INODE); // Size of the Inode Vector (-1 because of root)
    int sizeOfBlockVector = numBlocks * blockSize;     // Size of the Block Vector
    char sizeOfBitmap = ceil((numBlocks - 1) / 8.0);   // Size of the Bitmap (-1 because of root)

    char blockValues[sizeOfBlockVector];
    int goTo = 3 + sizeOfBitmap + sizeOfInodeVector + 1;
    fseek(file, goTo, SEEK_SET);
    fread(&blockValues, sizeOfChar, sizeof(blockValues), file); // Read the block vector value

    int freeBlockIndex; // Index of a free block
    for (size_t i = 0; i < sizeof(blockValues); i += blockSize)
    {
        if (blockValues[i] == 0)
        {
            freeBlockIndex = i / 2;
            break;
        }
    }

    INODE inode;
    int freeInodeIndex; // Index of a free inode
    goTo = 3 + sizeOfBitmap;
    fseek(file, goTo, SEEK_SET);
    for (int i = 0; i < numInodes; i++)
    {
        fread(&inode, sizeof(INODE), 1, file); // Read inode to find an empty one

        if (inode.IS_USED == 0)
        {
            freeInodeIndex = i;
            break;
        }
    }

    char dirName[10]; // Directory path name (skip "/")
    for (size_t i = 0; i < sizeof(dirName); i++)
    {
        if (i + 1 < strlen(dirPath.c_str()) && i + 1 < strlen(dirPath.c_str()))
            dirName[i] = dirPath.at(i + 1);
        else
            dirName[i] = 0;
    }

    goTo = 3 + sizeOfBitmap + freeInodeIndex * sizeof(INODE);
    char setDirectory[] = {0x01, 0x01};
    fseek(file, goTo, SEEK_SET);
    fwrite(&setDirectory, sizeOfChar, 2, file); // Set as directory
    fwrite(&dirName, sizeOfChar, 10, file);     // Set the name of directory
    fseek(file, 1, SEEK_CUR);
    fwrite(&freeBlockIndex, sizeOfChar, 1, file); // Set the index of block

    int numberOfBlocks = 1;
    goTo = 3 + sizeOfBitmap;
    fseek(file, goTo, SEEK_SET);
    int numberOfInodesFilled = -1;
    for (int i = 0; i < numInodes; i++)
    {
        fread(&inode, sizeof(INODE), 1, file); // Read inode to find how much blocks there are
        if (inode.IS_USED == 1)
        {
            numberOfInodesFilled++;

            if (inode.DIRECT_BLOCKS[0] > 0)
                numberOfBlocks++;
            if (inode.DIRECT_BLOCKS[1] > 0)
                numberOfBlocks++;
            if (inode.DIRECT_BLOCKS[2] > 0)
                numberOfBlocks++;
        }
    }

    int bitMap = 0;
    for (size_t i = 0; i < numberOfBlocks; i++)
    {
        bitMap += pow(2, i);
    }

    fseek(file, 3, SEEK_SET);
    fwrite(&bitMap, sizeOfChar, 1, file); // Add the bitmap

    goTo = 3 + sizeOfBitmap + 12;
    fseek(file, goTo, SEEK_SET);
    fwrite(&numberOfInodesFilled, sizeOfChar, 1, file); // Add the number of inodes in directory

    int valueInRootBlock = 2;
    goTo = 3 + sizeOfBitmap + sizeOfInodeVector + 2;
    fseek(file, goTo, SEEK_SET);
    fwrite(&valueInRootBlock, sizeOfChar, 1, file); // Add the number of inodes in directory block

    fclose(file);
}

void remove(std::string fsFileName, std::string path)
{
    FILE *file = fopen(fsFileName.c_str(), "r+");

    char blockSize, numBlocks, numInodes, tempValues[3];
    size_t sizeOfChar = sizeof(char); // Size of a char

    fread(&tempValues, sizeOfChar, 3, file); // Read block size, number of blocks and number of inodes
    blockSize = tempValues[0];
    numBlocks = tempValues[1];
    numInodes = tempValues[2];

    char sizeOfBitmap = ceil(numBlocks / 8.0);         // Size of the Bitmap
    int sizeOfInodeVector = numInodes * sizeof(INODE); // Size of the Inode Vector
    int sizeOfBlockVector = numBlocks * blockSize;     // Size of the Block Vector

    int directoriesCounter = 0, slashIndex = 0; // Number of directories and index of /
    char directoryName[10], pathName[10];       // Name of the path and directory

    for (size_t i = 0; i < strlen(path.c_str()); i++)
    {
        if (path.at(i) == '/')
        {
            directoriesCounter++;
            slashIndex = i;
        }
    }

    // Define directory and path names

    bool blockDirectoryName = false, blockPathName = false;
    for (size_t i = 0; i < sizeof(pathName); i++)
    {
        if (directoriesCounter == 1)
        {
            if ((i + 1) < strlen(path.c_str()))
                pathName[i] = path.at(i + 1);
            else
                pathName[i] = 0;

            if ((i + 1) < strlen(path.c_str()) && path.at(i) == '/')
                directoryName[i] = '/';
            else
                directoryName[i] = 0;
        }

        else
        {
            if ((i + 1) < strlen(path.c_str()) && path.at(i + 1) != '/' && !blockDirectoryName)
                directoryName[i] = path.at(i + 1);
            else
            {
                directoryName[i] = 0;
                blockDirectoryName = true;
            }

            if ((slashIndex + 1) < strlen(path.c_str()) && path.at(slashIndex + 1) != '/' && !blockPathName)
                pathName[i] = path.at(slashIndex + 1);
            else
            {
                pathName[i] = 0;
                blockPathName = true;
            }
            slashIndex++;
        }
    }

    // Find file and directory inode index

    INODE inode;
    int goTo = 3 + sizeOfBitmap;
    fseek(file, goTo, SEEK_SET);
    int indexOfPath = 0, indexOfDirectory = 0;
    for (int i = 0; i < numInodes; i++)
    {
        fread(&inode, sizeof(INODE), 1, file); // Read inode to delete it

        if (!strcmp(inode.NAME, pathName))
            indexOfPath = i;

        if (!strcmp(inode.NAME, directoryName))
            indexOfDirectory = i;
    }

    // Delete INODE

    goTo = 3 + sizeOfBitmap + sizeof(INODE) * indexOfPath;
    char emptyInode[22];
    for (size_t i = 0; i < sizeof(INODE); i++)
    {
        emptyInode[i] = 0;
    }
    fseek(file, goTo, SEEK_SET);
    fwrite(&emptyInode, sizeof(INODE), 1, file);

    // Decrease directory size

    char sizeOfDirectory = 0;
    goTo = 3 + sizeOfBitmap + indexOfDirectory * sizeof(INODE) + 12;
    fseek(file, goTo, SEEK_SET);
    fread(&sizeOfDirectory, sizeOfChar, 1, file);
    fseek(file, -1, SEEK_CUR);
    sizeOfDirectory--;
    fwrite(&sizeOfDirectory, sizeOfChar, 1, file); // Add directory size

    // Decrease or not the bitmap size

    INODE newInode;

    char bitMapValue[numBlocks];
    for (size_t i = 0; i < sizeof(bitMapValue); i++)
    {
        if (i == 0)
            bitMapValue[i] = 1;
        else
            bitMapValue[i] = 0;
    }
    goTo = 3 + sizeOfBitmap;
    fseek(file, goTo, SEEK_SET);
    for (int i = 0; i < numInodes; i++)
    {
        fread(&newInode, sizeof(INODE), 1, file); // Read inode to count blocks

        for (size_t j = 0; j < sizeof(newInode.DIRECT_BLOCKS); j++)
        {
            if (newInode.DIRECT_BLOCKS[j] != 0)
                bitMapValue[newInode.DIRECT_BLOCKS[j]] = 1;
        }
    }

    int valueToHex = 0; // Value of filled blocks in binary to dacimal
    for (size_t i = 0; i < sizeof(bitMapValue); i++)
    {
        if (bitMapValue[i] != 0)
        {
            valueToHex += pow(2, i);
        }
    }

    fseek(file, 3, SEEK_SET);
    fwrite(&valueToHex, sizeOfChar, 1, file); // Add the number of block filled in bitmap

    // Quick fix to save index of inode in the root inode
    int filledInodeCounter = -1;
    char indexOfInodeUsed = 0;
    goTo = 3 + sizeOfBitmap;
    fseek(file, goTo, SEEK_SET);
    for (int i = 0; i < numInodes; i++)
    {
        fread(&newInode, sizeof(INODE), 1, file); // Read inode to count blocks

        if (newInode.IS_USED == 1)
        {
            filledInodeCounter++;
            indexOfInodeUsed = i;
        }
    }

    if (filledInodeCounter == 1)
    {
        goTo = 3 + sizeOfBitmap + sizeOfInodeVector + 1;
        fseek(file, goTo, SEEK_SET);
        fwrite(&indexOfInodeUsed, sizeOfChar, 1, file);
    }

    fclose(file);
}

void move(std::string fsFileName, std::string oldPath, std::string newPath)
{

    FILE *file = fopen(fsFileName.c_str(), "r+");

    char blockSize, numBlocks, numInodes, tempValues[3];
    size_t sizeOfChar = sizeof(char); // Size of a char

    fread(&tempValues, sizeOfChar, 3, file); // Read block size, number of blocks and number of inodes
    blockSize = tempValues[0];
    numBlocks = tempValues[1];
    numInodes = tempValues[2];

    char sizeOfBitmap = ceil(numBlocks / 8.0);         // Size of the Bitmap
    int sizeOfInodeVector = numInodes * sizeof(INODE); // Size of the Inode Vector
    int sizeOfBlockVector = numBlocks * blockSize;     // Size of the Block Vector

    // Names of old path and directory
    int directoriesCounter = 0, slashIndex = 0; // Number of directories and index of /
    char directoryName[10], pathName[10];       // Name of the path and directory

    for (size_t i = 0; i < strlen(oldPath.c_str()); i++)
    {
        if (oldPath.at(i) == '/')
        {
            directoriesCounter++;
            slashIndex = i;
        }
    }

    bool blockDirectoryName = false, blockPathName = false;
    for (size_t i = 0; i < sizeof(pathName); i++)
    {
        if (directoriesCounter == 1)
        {
            if ((i + 1) < strlen(oldPath.c_str()))
                pathName[i] = oldPath.at(i + 1);
            else
                pathName[i] = 0;

            if ((i + 1) < strlen(oldPath.c_str()) && oldPath.at(i) == '/')
                directoryName[i] = '/';
            else
                directoryName[i] = 0;
        }

        else
        {
            if ((i + 1) < strlen(oldPath.c_str()) && oldPath.at(i + 1) != '/' && !blockDirectoryName)
                directoryName[i] = oldPath.at(i + 1);
            else
            {
                directoryName[i] = 0;
                blockDirectoryName = true;
            }

            if ((slashIndex + 1) < strlen(oldPath.c_str()) && oldPath.at(slashIndex + 1) != '/' && !blockPathName)
                pathName[i] = oldPath.at(slashIndex + 1);
            else
            {
                pathName[i] = 0;
                blockPathName = true;
            }
            slashIndex++;
        }
    }

    // Names of new path and directory
    directoriesCounter = 0;
    slashIndex = 0;                             // Number of directories and index of /
    char newDirectoryName[10], newPathName[10]; // Name of the path and directory

    for (size_t i = 0; i < strlen(newPath.c_str()); i++)
    {
        if (newPath.at(i) == '/')
        {
            directoriesCounter++;
            slashIndex = i;
        }
    }

    bool blocknewDirectoryName = false, blocknewPathName = false; // Booleans to block saving names to previne get trash
    for (size_t i = 0; i < sizeof(newPathName); i++)
    {
        if (directoriesCounter == 1)
        {
            if ((i + 1) < strlen(newPath.c_str()))
                newPathName[i] = newPath.at(i + 1);
            else
                newPathName[i] = 0;

            if ((i + 1) < strlen(newPath.c_str()) && newPath.at(i) == '/')
                newDirectoryName[i] = '/';
            else
                newDirectoryName[i] = 0;
        }

        else
        {
            if ((i + 1) < strlen(newPath.c_str()) && newPath.at(i + 1) != '/' && !blocknewDirectoryName)
                newDirectoryName[i] = newPath.at(i + 1);
            else
            {
                newDirectoryName[i] = 0;
                blocknewDirectoryName = true;
            }

            if ((slashIndex + 1) < strlen(newPath.c_str()) && newPath.at(slashIndex + 1) != '/' && !blocknewPathName)
                newPathName[i] = newPath.at(slashIndex + 1);
            else
            {
                newPathName[i] = 0;
                blocknewPathName = true;
            }
            slashIndex++;
        }
    }

    // Find new and old directories and path index
    INODE inode;                 // Inode to read inodes from file
    int goTo = 3 + sizeOfBitmap; // Index to use with fseek
    fseek(file, goTo, SEEK_SET);
    int indexOfPath = 0, indexOfOldDirectory = 0, indexOfNewDirectory = 0;
    for (int i = 0; i < numInodes; i++)
    {
        fread(&inode, sizeof(INODE), 1, file); // Read inode to delete it

        if (!strcmp(inode.NAME, pathName))
            indexOfPath = i;

        if (!strcmp(inode.NAME, directoryName))
            indexOfOldDirectory = i;

        if (!strcmp(inode.NAME, newDirectoryName))
            indexOfNewDirectory = i;
    }

    // If it's just a rename method, rename it, or else really move
    if (strcmp(pathName, newPathName))
    {
        int goTo = 3 + sizeOfBitmap + indexOfPath * sizeof(INODE) + 2;
        fseek(file, goTo, SEEK_SET);
        for (size_t i = 0; i < sizeof(newPathName); i++)
        {
            char newName = newPathName[i];
            fwrite(&newName, sizeOfChar, 1, file);
        }
    }
    else
    {
        // Decrease old direcoty size
        char sizeOfDirectory = 0;
        goTo = 3 + sizeOfBitmap + indexOfOldDirectory * sizeof(INODE) + 12;
        fseek(file, goTo, SEEK_SET);
        fread(&sizeOfDirectory, sizeOfChar, 1, file); // Read the currenty size of the directory
        sizeOfDirectory--;
        fseek(file, -1, SEEK_CUR);
        fwrite(&sizeOfDirectory, sizeOfChar, 1, file); // Decrease the directory size

        // Update blocks of removed inode directory
        goTo = 3 + sizeOfBitmap + indexOfOldDirectory * sizeof(INODE);
        INODE inodeToRemoveBlock;
        fseek(file, goTo, SEEK_SET);
        fread(&inodeToRemoveBlock, sizeof(INODE), 1, file);
        int usedBlocks = 0;
        for (size_t i = 0; i < sizeof(inodeToRemoveBlock.DIRECT_BLOCKS); i++)
        {
            if (inodeToRemoveBlock.DIRECT_BLOCKS[i] != 0)
            {
                usedBlocks++;
            }
        }
        if (inodeToRemoveBlock.NAME[0] == '/')
        {
            usedBlocks++;
        }

        char listOfValuesOfBlock[usedBlocks * blockSize]; // An array of char to manipulate the old directory's block(s)
        for (size_t i = 0; i < sizeof(listOfValuesOfBlock); i++)
        {
            listOfValuesOfBlock[i] = 0;
        }

        int increaseIndex = 0; // An index to manipulate the listOfValuesOfBlock
        for (size_t i = 0; i < usedBlocks; i++)
        {
            char tempValues[blockSize];
            goTo = 3 + sizeOfBitmap + sizeOfInodeVector + 1 + inodeToRemoveBlock.DIRECT_BLOCKS[i] * blockSize;
            fseek(file, goTo, SEEK_SET);
            fread(&tempValues, blockSize, 1, file);

            for (size_t j = 0; j < blockSize; j++)
            {
                listOfValuesOfBlock[j + increaseIndex] = tempValues[j];
            }
            increaseIndex = blockSize;
        }

        for (size_t i = 0; i < sizeof(listOfValuesOfBlock); i++)
        {
            if (i + 1 < sizeof(listOfValuesOfBlock) && listOfValuesOfBlock[i + 1] != 0)
            {
                if (listOfValuesOfBlock[i] == indexOfPath)
                    listOfValuesOfBlock[i] = listOfValuesOfBlock[i + 1];

                if (i != 0)
                    if (listOfValuesOfBlock[i] == listOfValuesOfBlock[i - 1])
                        listOfValuesOfBlock[i] = listOfValuesOfBlock[i + 1];
            }
        }

        increaseIndex = 0;
        for (size_t i = 0; i < usedBlocks; i++)
        {
            goTo = 3 + sizeOfBitmap + sizeOfInodeVector + 1 + inodeToRemoveBlock.DIRECT_BLOCKS[i] * blockSize;
            fseek(file, goTo, SEEK_SET);

            for (size_t j = 0; j < blockSize; j++)
            {
                char tempValue = listOfValuesOfBlock[j + increaseIndex];
                fwrite(&tempValue, sizeOfChar, 1, file); // Update the value of old directory block
            }
            increaseIndex = blockSize;
        }

        // Update Inode blocks if necessary
        if (usedBlocks * 2 > sizeOfDirectory)
        {
            for (size_t i = sizeOfDirectory - 1; i < sizeof(inodeToRemoveBlock.DIRECT_BLOCKS); i++)
            {
                inodeToRemoveBlock.DIRECT_BLOCKS[i] = 0;
            }
            goTo = 3 + sizeOfBitmap + indexOfOldDirectory * sizeof(INODE);
            fseek(file, goTo, SEEK_SET);
            fwrite(&inodeToRemoveBlock, sizeof(INODE), 1, file); // Remove unnecessary block(s)
        }

        // Increase the new directory size
        sizeOfDirectory = 0;
        goTo = 3 + sizeOfBitmap + indexOfNewDirectory * sizeof(INODE) + 12;
        fseek(file, goTo, SEEK_SET);
        fread(&sizeOfDirectory, sizeOfChar, 1, file);
        sizeOfDirectory++;
        fseek(file, -1, SEEK_CUR);
        fwrite(&sizeOfDirectory, sizeOfChar, 1, file); // Increase the new directory size

        // Find a free block to save the content of file
        goTo = 3 + sizeOfBitmap + indexOfNewDirectory * sizeof(INODE);
        INODE newInode;
        fseek(file, goTo, SEEK_SET);
        fread(&newInode, sizeof(INODE), 1, file);
        int lastBlockUsed = 0;
        for (size_t i = 0; i < sizeof(newInode.DIRECT_BLOCKS); i++)
        {
            if (newInode.DIRECT_BLOCKS[i] != 0 && indexOfPath != newInode.DIRECT_BLOCKS[i])
                lastBlockUsed = i;
        }
        int tempSize = sizeOfDirectory - 1;
        while (tempSize > blockSize)
        {
            tempSize -= blockSize;
        }
        int indexOfDirectoryBlock = newInode.DIRECT_BLOCKS[lastBlockUsed] * 2 + tempSize;

        // Veryfi the necessity of increase the size of the new directory block and add this block
        if (sizeOfDirectory > blockSize)
        {
            char freeBlockIndex = 0; // Index of a free block
            goTo = 3 + sizeOfBitmap;
            fseek(file, goTo, SEEK_SET);
            for (int i = 0; i < numInodes; i++)
            {
                fread(&inode, sizeof(INODE), 1, file); // Read inode to count blocks

                for (size_t j = 0; j < sizeof(inode.DIRECT_BLOCKS); j++)
                {
                    if (inode.DIRECT_BLOCKS[j] > freeBlockIndex)
                    {
                        freeBlockIndex = inode.DIRECT_BLOCKS[j];
                    }
                }
            }
            freeBlockIndex++;

            indexOfDirectoryBlock = freeBlockIndex * 2;

            goTo = 3 + sizeOfBitmap + indexOfNewDirectory * sizeof(INODE);
            fseek(file, goTo, SEEK_SET);
            fread(&newInode, sizeof(INODE), 1, file);
            for (size_t i = 1; i < sizeof(newInode.DIRECT_BLOCKS); i++)
            {
                if (newInode.DIRECT_BLOCKS[i] == 0)
                {
                    newInode.DIRECT_BLOCKS[i] = freeBlockIndex;
                    break;
                }
            }
            fseek(file, goTo, SEEK_SET);
            fwrite(&newInode, sizeof(INODE), 1, file);
        }

        goTo = 3 + sizeOfBitmap + sizeOfInodeVector + 1 + indexOfDirectoryBlock;
        fseek(file, goTo, SEEK_SET);
        fwrite(&indexOfPath, sizeOfChar, 1, file); // Add the index of inode file to the directory block

        // Update the bitmap
        char bitMapValue[numBlocks]; // Array of char to manipulate the bitmap
        for (size_t i = 0; i < sizeof(bitMapValue); i++)
        {
            if (i == 0)
                bitMapValue[i] = 1; // Starts with 1 because of root directory
            else
                bitMapValue[i] = 0;
        }
        goTo = 3 + sizeOfBitmap;
        fseek(file, goTo, SEEK_SET);
        for (int i = 0; i < numInodes; i++)
        {
            fread(&newInode, sizeof(INODE), 1, file); // Read inode to count blocks

            for (size_t j = 0; j < sizeof(newInode.DIRECT_BLOCKS); j++)
            {
                if (newInode.DIRECT_BLOCKS[j] != 0)
                    bitMapValue[newInode.DIRECT_BLOCKS[j]] = 1;
            }
        }

        int valueToHex = 0; // Value of filled blocks in binary to dacimal
        for (size_t i = 0; i < sizeof(bitMapValue); i++)
        {
            if (bitMapValue[i] != 0)
                valueToHex += pow(2, i);
        }

        fseek(file, 3, SEEK_SET);
        fwrite(&valueToHex, sizeOfChar, 1, file); // Add the number of block filled (bitmap)
    }
    fclose(file);
}
