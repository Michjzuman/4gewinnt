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
|human|The Interface for a human to play|
|jonkler|A Bot that makes random moves|
|bot2|A Bot that predicts 2 moves|
|bot3|A Bot that predicts 3 moves|
|bot4|A Bot that predicts 4 moves|
|bot5|A Bot that predicts 5 moves (requires a good CPU with at least 8 Cores)|
|slop_bot|A Bot that was vibecoded with GPT-5.5|
|gemma4|Gemma4:e2b over Ollama (needs to be installed via Ollama)|