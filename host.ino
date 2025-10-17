#define MAX_ROWS 26
#define MAX_COLS 26

int rows = 0;
int cols = 0;
int mapMatrix[MAX_ROWS][MAX_COLS];
bool gameActive = false;
int ship1Count = 0;
int ship2Count = 0;
int ship3Count = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("Arduino Ready");
  Serial.println("Waiting for START command...");
  randomSeed(analogRead(A0));
}

void loop() {
  if (!gameActive) {
    if (Serial.available()) {
      ship1Count = 0;
      ship2Count = 0;
      ship3Count = 0;
      String msg = Serial.readStringUntil('\n');
      msg.trim();
      
      // Parse "Start x y ship1 ship2 ship3"
      if (msg.startsWith("Start ")) {
        int idx = 6; // Skip "Start "
        
        // Parse X (columns)
        int nextSpace = msg.indexOf(' ', idx);
        if (nextSpace == -1) return;
        cols = msg.substring(idx, nextSpace).toInt();
        idx = nextSpace + 1;
        
        // Parse Y (rows)
        nextSpace = msg.indexOf(' ', idx);
        if (nextSpace == -1) return;
        rows = msg.substring(idx, nextSpace).toInt();
        idx = nextSpace + 1;
        
        // Parse ship1 count
        nextSpace = msg.indexOf(' ', idx);
        if (nextSpace == -1) return;
        ship1Count = msg.substring(idx, nextSpace).toInt();
        idx = nextSpace + 1;
        
        // Parse ship2 count
        nextSpace = msg.indexOf(' ', idx);
        if (nextSpace == -1) return;
        ship2Count = msg.substring(idx, nextSpace).toInt();
        idx = nextSpace + 1;
        
        // Parse ship3 count
        ship3Count = msg.substring(idx).toInt();
        
        // Validate dimensions
        if (rows > 0 && rows <= MAX_ROWS && cols > 0 && cols <= MAX_COLS) {
          Serial.println("OK");
          generateMapWithShips();
          sendMap();
          gameActive = true;
        } else {
          Serial.println("ERROR: Invalid dimensions");
        }
      }
    }
  } else {
    // Game loop
    if (Serial.available()) {
      String guess = Serial.readStringUntil('\n');
      guess.trim();
      
      // Check for RESET command
      if (guess.equals("RESET")) {
        Serial.println("GAME OVER");
        sendMap();
        gameActive = false;
        Serial.println("Arduino Ready");
        Serial.println("Waiting for START command...");
      } else {
        handleGuess(guess);
        sendMap();
      }
    }
  }
}

// === Helper functions ===

void generateMapWithShips() {
  // Initialize map to empty (0)
  for (int r = 0; r < rows; r++) {
    for (int c = 0; c < cols; c++) {
      mapMatrix[r][c] = 0;
    }
  }
  
  // Place ships: size 1, 2, and 3
  placeShips(1, ship1Count);
  placeShips(2, ship2Count);
  placeShips(3, ship3Count);
}

void placeShips(int shipSize, int count) {
  for (int i = 0; i < count; i++) {
    bool placed = false;
    int attempts = 0;
    
    while (!placed && attempts < 100) {
      attempts++;
      
      int r = random(0, rows);
      int c = random(0, cols);
      bool horizontal = random(0, 2) == 0;
      
      if (horizontal) {
        // Check if ship fits horizontally
        if (c + shipSize > cols) continue;
        
        // Check if positions are empty
        bool canPlace = true;
        for (int j = 0; j < shipSize; j++) {
          if (mapMatrix[r][c + j] != 0) {
            canPlace = false;
            break;
          }
        }
        
        if (canPlace) {
          // Place ship
          for (int j = 0; j < shipSize; j++) {
            mapMatrix[r][c + j] = 1;
          }
          placed = true;
        }
      } else {
        // Check if ship fits vertically
        if (r + shipSize > rows) continue;
        
        // Check if positions are empty
        bool canPlace = true;
        for (int j = 0; j < shipSize; j++) {
          if (mapMatrix[r + j][c] != 0) {
            canPlace = false;
            break;
          }
        }
        
        if (canPlace) {
          // Place ship
          for (int j = 0; j < shipSize; j++) {
            mapMatrix[r + j][c] = 1;
          }
          placed = true;
        }
      }
    }
  }
}

void generateMap() {
  for (int r = 0; r < rows; r++) {
    for (int c = 0; c < cols; c++) {
      mapMatrix[r][c] = random(0, 2);
    }
  }
}

// Send the visible map: all hidden '~', only hits shown as 'X'
void sendMap() {
  Serial.println("MAP");
  for (int r = 0; r < rows; r++) {
    for (int c = 0; c < cols; c++) {
      if (mapMatrix[r][c] == 2)
        Serial.print("x ");       // hit revealed
      else
        Serial.print("~ ");       // hidden
    }
    Serial.println();
  }
  Serial.println("END");
}

void handleGuess(String pos) {
  if (pos.length() < 2) return;
  char rowChar = toupper(pos.charAt(0));
  int col = pos.substring(1).toInt() - 1;
  int row = rowChar - 'A';

  if (row >= 0 && row < rows && col >= 0 && col < cols) {
    if (mapMatrix[row][col] == 1) {
      mapMatrix[row][col] = 2; // mark as hit
      if(isGameOver()) {
        Serial.println("GAME OVER");
        gameActive = false;
      }else {
        Serial.println("HIT!");
      }
    } else if (mapMatrix[row][col] == 0) {
      Serial.println("MISS!");
    }else {
      Serial.println("ALREADY TRIED!");
    }
  } else {
    Serial.println("INVALID!");
  }
}

bool isGameOver() {
  for (int r = 0; r < rows; r++) {
    for (int c = 0; c < cols; c++) {
      if (mapMatrix[r][c] == 1) return false; // any ship left?
    }
  }
  return true;
}
