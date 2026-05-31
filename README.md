# 4gewinnt

This is a simple implementation of the classic `4 in a row`-game in C.

To start it you have to manually compile it first:
```
clang game.c -o 4gewinnt
```

And then you run it:
```
./4gewinnt
```

### The different players explained
|Name|Explanation|
|---|---|
|human|The interface for a human to play|
|jonkler|A bot that makes random moves|
|bot2|A bot that predicts 2 moves|
|bot3|A bot that predicts 3 moves|
|bot4|A bot that predicts 4 moves|
|bot5|A bot that predicts 5 moves (requires a good CPU with at least 8 cores)|
|slop_bot|A bot that was vibecoded with GPT-5.5|
|gemma4|Gemma4:e2b over ollama (needs to be installed via ollama)|