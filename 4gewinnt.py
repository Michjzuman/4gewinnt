import random
import time
import os

class Game:
    def __init__(self):
        self.board = [
            [0 for _ in range(7)]
            for _ in range(6)
        ]
    
    def draw(self):
        print()
        for line in self.board:
            print("|" + "|".join([f"""{
                ['_', '\033[31m#', '\033[33m#'][row]
            }\033[0m""" for row in line]) + "|")
        print()

    def move(self, player: int, x: int) -> bool:
        
        for y, line in enumerate(self.board):
            if self.board[len(self.board) - y - 1][x] == 0:
                self.board[len(self.board) - y - 1][x] = player
                return True
        
        return False

    def winner(self) -> int:
        height = len(self.board)
        width = len(self.board[0]) if height else 0
        directions = [
            (0, 1),   # horizontal
            (1, 0),   # vertical
            (1, 1),   # diagonal down-right
            (-1, 1),  # diagonal up-right
        ]

        for y in range(height):
            for x in range(width):
                player = self.board[y][x]
                if player == 0:
                    continue

                for dy, dx in directions:
                    if all(
                        0 <= y + dy * step < height
                        and 0 <= x + dx * step < width
                        and self.board[y + dy * step][x + dx * step] == player
                        for step in range(4)
                    ):
                        return player

        return 0

def run()

if __name__ == "__main__":
    game = Game()
    turn = 1
    while game.winner() == 0:
        os.system("clear; clear")
        game.move(turn, random.randint(0, 6))
        game.draw()
        turn = turn % 2 + 1
        time.sleep(0.2)
    
