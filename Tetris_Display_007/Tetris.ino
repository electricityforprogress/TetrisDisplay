


float Normalize(int min, int max, int value)
{
  float result = -1.0 + (float)((value - min) << 1) / (max - min);
  return result < -1 ? -1 : result > 1 ? 1 : result;
}

void clearLEDs()
{
  matrix.fillScreen(0);
}


void setColor( int r, int c, unsigned long color)
{
    if(white && displayMode && color != BLACK) color = WHITE; //draw white in game mode only
    matrix.drawPixel(r, c, color);

}


void render()
{
  int value = 0;
  unsigned long color = 0;


  //render game field first
  for( int row = 0; row < GAME_ROWS; row++)
  {
    for( int col = 0; col < GAME_COLUMNS; col++)
    {
      color = 0;
      value = gameField[row * GAME_COLUMNS+col];
      if( value > 0) color = colors[value-1];
       setColor(row,col, color);
    }    
  }

  //render falling piece
  for( int row = 0; row < fallingPiece->Rows; row++)
  {
    for( int col = 0; col < fallingPiece->Columns; col++)
    {
        value = fallingPiece->getV(row,col);
        if( value > 0) setColor(currentRow+row,currentColumn+col, colors[value-1]);
    }
  }

  /*
  //render divider line
    for( int row = 0; row < LED_ROWS; row++)
    {
      for( int col = GAME_COLUMNS; col < LED_COLUMNS; col ++)
      {
        if( col == GAME_COLUMNS )
          setColor(row,col, WHITE);
        else
          setColor(row,col, 0);
      }
    }
   
    //render next piece
    for( int row = 0; row < nextPiece->Rows; row++)
    {
      for( int col = 0; col < nextPiece->Columns; col++)
      {
        value = (*nextPiece)(row,col);
        if( value > 0)    
          setColor(7+row,12+col, colors[value-1]);
        else
          setColor(7+row,12+col, 0);
      }
    }  
   */

//might want to slow this down...
  matrix.show();

  //serial monitor print the board
//write it all to a buffer and then print it
  boolean buffer[GAME_ROWS][GAME_COLUMNS];
  //clear buffer
  for( int row = 0; row < GAME_ROWS; row++)
  {
    for( int col = 0; col < GAME_COLUMNS; col++)
    {      
      buffer[row][col]=false;
    }
  }
  
  for( int row = 0; row < fallingPiece->Rows; row++)
  {
    for( int col = 0; col < fallingPiece->Columns; col++)
    {
        value = fallingPiece->getV(row,col);
        if( value > 0) buffer[currentRow+row][currentColumn+col] = true; 
    }
  }

//draw playfield
    Serial.println("_ ---------- _");
  for( int row = 0; row < GAME_ROWS; row++)
  {
    Serial.print("| ");
    for( int col = 0; col < GAME_COLUMNS; col++)
    {      
      color = 0;
      value = gameField[row * GAME_COLUMNS+col];
      if(buffer[row][col]) Serial.print("X");
      else{
        if( value > 0) color = colors[value-1];
          if(color == 0) Serial.print(".");
          else Serial.print("#");
        }  
    }
    
    Serial.println(" |");
  }
      Serial.println("- ========== -");
}



void startGame()
{
  Serial.println("Start game");

  clearLEDs();


  for(int i = 15;i >= 0;i--)
  {
    for(int j = 0;j < 8;j++)  setColor(i, j, 255);
    matrix.show();
    delay(20);
    //tone(2,i*300);
    delay(10);
    for(int j = 0;j < 8;j++)  setColor(i, j, 0);
    //noTone(2);
  }
  matrix.show();

  //bar.clear();
  //bar.writeDisplay();

  nextPiece=NULL;
  gameLines = 0;
  loopStartTime = 0;
  newLevel(1);
  gameOver = false;
  render();
  /*tone(2,500);
  delay(200);
  tone(2,1000);
  delay(200);
  tone(2,500);
  delay(200);
  noTone(2);*/

}


void newLevel(uint8_t level)
{
  gameLevel = level;

  //bar.clear();
  //for(int b = 12;b > 12-level;b--) //bar.setBar(b-1, LED_RED);
  //bar.writeDisplay();

  if(gameLevel == 1)
  {
    emptyField();
  }

  newPiece();
}


void emptyField()
{
  for(int i = 0; i < GAME_ROWS * GAME_COLUMNS; i++ ) gameField[i] = 0;
}


void newPiece()
{
  int next;

  currentColumn = 4;
  currentRow = 0;

  if (nextPiece == NULL)
  {
    next = random(0,7);
    nextPiece = &_gamePieces[next];
  }

    if(fallingPiece != NULL) {free(fallingPiece->_data);delete fallingPiece;};
    fallingPiece = new GamePiece(*nextPiece);

  next = random(0,7);
  nextPiece = &_gamePieces[next];  

  //Serial.print("Next: "); Serial.println(next);
}


boolean isValidLocation(GamePiece & piece, byte column, byte row)
{
  for (int i = 0; i < piece.Rows; i++)
    for (int j = 0; j < piece.Columns; j++)
    {
      int newRow = i + row;
      int newColumn = j + column;                    

      //location is outside of the fieled
      if (newColumn < 0 || newColumn > GAME_COLUMNS - 1 || newRow < 0 || newRow > GAME_ROWS - 1)
      {
        //piece part in that location has a valid square - not good
        if (piece(i, j) != 0) return false;
      }
      else
      {
        //location is in the field but is already taken, pice part for that location has non-empty square 
        if (gameField[newRow*GAME_COLUMNS + newColumn] != 0 && piece(i, j) != 0) return false;
      }
    }

  return true;  
}


void moveDown()
{
  if (isValidLocation(*fallingPiece, currentColumn, currentRow + 1))
  {
    currentRow +=1;
    return;
  }


  //The piece can't be moved anymore, merge it into the game field
  for (int i = 0; i < fallingPiece->Rows; i++)
  {
    for (int j = 0; j < fallingPiece->Columns; j++)
    {
      byte value = (*fallingPiece)(i, j);
      if (value != 0) gameField[(i + currentRow) * GAME_COLUMNS + (j + currentColumn)] = value;
    }
  }

  //Piece is merged update the score and get a new pice
  updateScore();            
  newPiece();  

 // Serial.println("MoveDown");
}


void drop()
{
  
 // Serial.println("Drop");
  while (isValidLocation(*fallingPiece, currentColumn, currentRow + 1)) moveDown();
  loopStartTime = -500;
  delay(DLY);  
}

void moveLeft()
{
  if (isValidLocation(*fallingPiece, currentColumn - 1, currentRow)) currentColumn--;
  delay(DLY);  
}



void moveRight()
{
  if (isValidLocation(*fallingPiece, currentColumn + 1, currentRow)) currentColumn++;
  delay(DLY);  

}


void rotateRight()
{
    rotated = fallingPiece->rotateRight();

  if (isValidLocation(*rotated, currentColumn, currentRow)) 
  {
      free(fallingPiece->_data);
      delete fallingPiece;

      fallingPiece = rotated;
  } else {free(rotated->_data);delete rotated;};

  
  delay(DLY);  
}


void rotateLeft()
{
  GamePiece * rotated = fallingPiece->rotateLeft();

  if (isValidLocation(*rotated, currentColumn, currentRow)) 
  {
     delete fallingPiece;
     fallingPiece = rotated;
  }
  delay(DLY);  
}


void updateScore()
{
  int count = 0;
  for(int row = 1; row < GAME_ROWS; row++)
  {
    boolean goodLine = true;
    for (int col = 0; col < GAME_COLUMNS; col++) if(gameField[row *GAME_COLUMNS + col] == 0) goodLine = false;

    if(goodLine)
    {
      count++;
      for (int i = row; i > 0; i--)
      {
        //tone(2,300+(i*100));
        digitalWrite(4,HIGH);  // liga motor vibracao
        delay(10);
        for (int j = 0; j < GAME_COLUMNS; j++) gameField[i *GAME_COLUMNS +j] = gameField[(i - 1)*GAME_COLUMNS+ j];
        //noTone(2);
        digitalWrite(4,LOW);
        render();
      }
    }
  }


  if (count > 0)
  {
    //_gameScore += count * (_gameLevel * 10);
    gameLines += count;


    int nextLevel = (gameLines / GAME_ROWS) + 1;
    int t = gameLines-(gameLines/GAME_ROWS*16);
    t = 24 - map(t,1,16,1,13);
    //for(int b = 24;b >= t;b--) //bar.setBar(b, LED_GREEN);
    //bar.writeDisplay();


    if (nextLevel > gameLevel)
    {
      gameLevel = nextLevel;
      newLevel(gameLevel);
      /*tone(2,300);
      delay(100);
      tone(2,500);
      delay(200);
      tone(2,300);
      delay(100);
      tone(2,500);
      delay(200);
      noTone(2);*/
    }
  }
}

void exec_gameOver()
{
  delay(100);
  clearLEDs();

  for(int i = 0;i < 16;i++)
  {
    for(int j = 0;j < 8;j++)  setColor(i, j, 255);
    matrix.show();
    delay(20);
    //tone(2,i*300);
    delay(10);
    for(int j = 0;j < 8;j++)  setColor(i, j, 0);
    //noTone(2);
  }
  matrix.show();
Serial.println("GameOver");
  startGame();
}


int freeRam() 
{
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}
