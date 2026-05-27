curl -s http://localhost:11434/api/chat -H "Content-Type: application/json" -d '{
    
    "model": "gemma4:e2b-mlx",
    "message": "What is the temperature in New York?",

    "stream": false,
    "tools": [
        {
            "type": "function",
            "function": {
                "name": "get_temperature",
                "description": "Get the current temperature for a city",
                "parameters": {
                    "type": "object",
                    "required": ["city"],
                    "properties": {
                        "city": {
                            "type": "string",
                            "description": "The name of the city"
                        }
                    }
                }
            }
        }
    ]
}'