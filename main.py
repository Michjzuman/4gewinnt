import random
import time
import os

class Game:
    def __init__(self):
        self.board = [[0] * 7] * 6
    
    def draw(self):
        print()
        for line in self.board:
            print("|" + "|".join([f"""{
                ['_', '\033[31m#', '\033[33m#'][row]
            }\033[0m""" for row in line]) + "|")
        print()

    def move(self, player: int, x: int) -> bool:
        
        
        
        return True

if __name__ == "__main__":
    game = Game()
    os.system("clear; clear")
    game.move(1, 3)
    game.draw()
    