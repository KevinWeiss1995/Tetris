#include "tetrisboard.h"

const std::vector<std::vector<std::vector<bool>>> TetrisBoard::PIECES = {
    // Each piece is represented as a 4x4 matrix of booleans
    // Only the first rotation is stored - others are calculated
    
    // I - long piece
    {
        {0,0,0,0,
         1,1,1,1,
         0,0,0,0,
         0,0,0,0}
    },
    // O
    {
        {0,1,1,0,
         0,1,1,0,
         0,0,0,0,
         0,0,0,0}
    },
    // T
    {
        {0,1,0,0,
         1,1,1,0,
         0,0,0,0,
         0,0,0,0}
    },
    // S
    {
        {0,1,1,0,
         1,1,0,0,
         0,0,0,0,
         0,0,0,0}
    },
    // Z
    {
        {1,1,0,0,
         0,1,1,0,
         0,0,0,0,
         0,0,0,0}
    },
    // J
    {
        {1,0,0,0,
         1,1,1,0,
         0,0,0,0,
         0,0,0,0}
    },
    // L
    {
        {0,0,1,0,
         1,1,1,0,
         0,0,0,0,
         0,0,0,0}
    }
};

const std::vector<QColor> TetrisBoard::COLORS = {
    Qt::cyan,    // I
    Qt::yellow,  // O
    Qt::magenta, // T
    Qt::green,   // S
    Qt::red,     // Z
    Qt::blue,    // J
    Qt::darkYellow  // L
};

TetrisBoard::TetrisBoard(QWidget *parent) : QWidget(parent) {

    board.resize(BOARD_HEIGHT, std::vector<int>(BOARD_WIDTH, 0));

    initPiece(rand() % PIECES.size());
    
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &TetrisBoard::updateGame);
    timer->start(500);
    setFocusPolicy(Qt::StrongFocus);
}

void TetrisBoard::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    
    // Fill background
    painter.fillRect(rect(), Qt::black);
    
    // Draw board grid
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            QRect rect(x * BLOCK_SIZE, y * BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE);
            if (board[y][x] > 0) {
                painter.fillRect(rect, COLORS[board[y][x] - 1]);
            }
            painter.drawRect(rect);
        }
    }
    
    // Draw current piece
    for (int y = 0; y < PIECE_SIZE; y++) {
        for (int x = 0; x < PIECE_SIZE; x++) {
            if (currentPiece.shape[y][x]) {
                QRect rect((currentPiece.x + x) * BLOCK_SIZE,
                         (currentPiece.y + y) * BLOCK_SIZE,
                         BLOCK_SIZE, BLOCK_SIZE);
                painter.fillRect(rect, COLORS[currentPiece.type]);
            }
        }
    }
    
    // Draw score
    drawScore(painter);

    // Draw combo if active
    if (currentCombo.remainingTicks > 0) {
        QFont comboFont = painter.font();
        comboFont.setPointSize(32);
        comboFont.setBold(true);
        painter.setFont(comboFont);
        
        // Create a new color with alpha
        QColor fadeColor = currentCombo.color;
        fadeColor.setAlpha(qMax(0, (255 * currentCombo.remainingTicks) / COMBO_DISPLAY_TIME));
        painter.setPen(fadeColor);
        
        // Draw with slight offset for each frame to make it "float" up
        QPoint drawPos = currentCombo.position;
        drawPos.setY(drawPos.y() - (COMBO_DISPLAY_TIME - currentCombo.remainingTicks));
        
        painter.drawText(drawPos, currentCombo.text);
        currentCombo.remainingTicks--;
    }
}

void TetrisBoard::keyPressEvent(QKeyEvent *event) {
    if (gameOver) {
        if (event->key() == Qt::Key_Space) {
            startNewGame();
        }
        return;
    }

    switch (event->key()) {
    case Qt::Key_Left:
        moveCurrentPiece(-1, 0);
        break;
    case Qt::Key_Right:
        moveCurrentPiece(1, 0);
        break;
    case Qt::Key_Down:
        moveCurrentPiece(0, 1);
        break;
    case Qt::Key_Up:
        rotatePiece();
        break;
    case Qt::Key_Space:
        hardDrop();
        break;
    case Qt::Key_S:
        swapPiece();
        break;
    }
    update();
}

void TetrisBoard::updateGame() {
    if (!moveCurrentPiece(0, 1)) {
        lockPiece();
    }
    update();
}

bool TetrisBoard::moveCurrentPiece(int dx, int dy) {
    currentPiece.x += dx;
    currentPiece.y += dy;
    
    if (checkCollision()) {
        currentPiece.x -= dx;
        currentPiece.y -= dy;
        return false;
    }
    return true;
}

bool TetrisBoard::checkCollision() {
    for (int y = 0; y < PIECE_SIZE; y++) {
        for (int x = 0; x < PIECE_SIZE; x++) {
            if (currentPiece.shape[y][x]) {
                int boardX = currentPiece.x + x;
                int boardY = currentPiece.y + y;
                
                if (boardX < 0 || boardX >= BOARD_WIDTH || 
                    boardY >= BOARD_HEIGHT ||
                    (boardY >= 0 && board[boardY][boardX] > 0)) {
                    return true;
                }
            }
        }
    }
    return false;
}

void TetrisBoard::clearLines() {
    int linesCleared = 0;
    for (int y = BOARD_HEIGHT - 1; y >= 0; y--) {
        // Scan from bottom up, checking for complete lines
        bool lineFull = true;
        for (int x = 0; x < BOARD_WIDTH; x++) {
            if (board[y][x] == 0) {
                lineFull = false;
                break;
            }
        }
        
        if (lineFull) {
            linesCleared++;
            // Shift all lines above down by one position
            for (int moveY = y; moveY > 0; moveY--) {
                board[moveY] = board[moveY - 1];
            }
            // Reset top line and recheck current position since we moved everything down
            board[0] = std::vector<int>(BOARD_WIDTH, 0);
            y++;
        }
    }
    
    if (linesCleared > 0) {
        updateScore(linesCleared);
    }
}

void TetrisBoard::updateScore(int lines) {
    static const int POINTS[] = {0, 40, 100, 300, 1200};
    score += POINTS[lines] * level;
    linesCleared += lines;
    level = 1 + (linesCleared / 10);

    // Set combo display for multiple lines
    if (lines > 1) {
        QString comboText = QString("x%1").arg(lines);
        QColor comboColor;
        switch(lines) {
            case 2:
                comboColor = Qt::yellow;
                break;
            case 3:
                comboColor = Qt::cyan;
                break;
            case 4:
                comboColor = QColor(255, 50, 50);  // Bright red
                break;
        }
        
        // Position in center of board
        QPoint comboPos(
            (BOARD_WIDTH * BLOCK_SIZE) / 2,
            (BOARD_HEIGHT * BLOCK_SIZE) / 2
        );
        
        currentCombo = {comboText, comboColor, COMBO_DISPLAY_TIME, comboPos};
    }

    timer->setInterval(std::max(100, 500 - (level-1) * 20));
}

void TetrisBoard::drawScore(QPainter& painter) {
    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setPointSize(12);
    painter.setFont(font);
    
    // Draw score info
    QRect scoreRect(BOARD_WIDTH * BLOCK_SIZE + 10, 10, 100, 100);
    painter.drawText(scoreRect, Qt::AlignLeft, 
                    QString("Score:\n%1\n\nLevel:\n%2\n\nLines:\n%3")
                    .arg(score)
                    .arg(level)
                    .arg(linesCleared));

    // Draw saved piece
    if (hasSavedPiece) {
        QRect savedPieceRect(BOARD_WIDTH * BLOCK_SIZE + 10, 150, 
                            PIECE_SIZE * BLOCK_SIZE, PIECE_SIZE * BLOCK_SIZE);
        painter.drawText(QPoint(savedPieceRect.x(), savedPieceRect.y() - 5), "Saved:");
        
        for (int y = 0; y < PIECE_SIZE; y++) {
            for (int x = 0; x < PIECE_SIZE; x++) {
                if (savedPiece.shape[y][x]) {
                    QRect blockRect(savedPieceRect.x() + x * BLOCK_SIZE,
                                  savedPieceRect.y() + y * BLOCK_SIZE,
                                  BLOCK_SIZE, BLOCK_SIZE);
                    painter.fillRect(blockRect, COLORS[savedPiece.type]);
                    painter.drawRect(blockRect);
                }
            }
        }
    }

    // Game over message
    if (gameOver) {
        font.setPointSize(20);
        painter.setFont(font);
        QRect messageRect(0, BOARD_HEIGHT * BLOCK_SIZE / 2 - 50,
                         BOARD_WIDTH * BLOCK_SIZE, 100);
        painter.drawText(messageRect, Qt::AlignCenter,
                        "Game Over!\nPress Space\nto Restart");
    }
}

void TetrisBoard::lockPiece() {
    for (int y = 0; y < PIECE_SIZE; y++) {
        for (int x = 0; x < PIECE_SIZE; x++) {
            if (currentPiece.shape[y][x]) {
                int boardY = currentPiece.y + y;
                if (boardY >= 0) {
                    board[boardY][currentPiece.x + x] = currentPiece.type + 1;
                }
            }
        }
    }
    clearLines();
    initPiece(rand() % PIECES.size());
    checkGameOver();
}

void TetrisBoard::rotatePiece() {
    int newRotation = (currentPiece.rotation + 1) % 4;
    auto oldShape = currentPiece.shape;
    currentPiece.shape = getRotatedShape(currentPiece.type, newRotation);
    
    if (checkCollision()) {
        currentPiece.shape = oldShape;
    } else {
        currentPiece.rotation = newRotation;
    }
}

void TetrisBoard::initPiece(int type) {
    currentPiece.type = type;
    currentPiece.rotation = 0;
    currentPiece.x = BOARD_WIDTH / 2 - 2;
    currentPiece.y = 0;
    currentPiece.shape = getRotatedShape(type, 0);
}

std::vector<std::vector<bool>> TetrisBoard::getRotatedShape(int type, int rotation) {
    // Convert the 1D piece definition into a 2D matrix for easier rotation
    std::vector<std::vector<bool>> shape(PIECE_SIZE, std::vector<bool>(PIECE_SIZE));
    std::vector<bool> original = PIECES[type][0];
    
    for (int y = 0; y < PIECE_SIZE; y++) {
        for (int x = 0; x < PIECE_SIZE; x++) {
            shape[y][x] = original[y * PIECE_SIZE + x];
        }
    }
    
    // Apply clockwise rotations by mapping coordinates:
    // (x,y) -> (y, PIECE_SIZE-1-x) for each 90-degree turn
    for (int r = 0; r < rotation; r++) {
        std::vector<std::vector<bool>> rotated = shape;
        for (int y = 0; y < PIECE_SIZE; y++) {
            for (int x = 0; x < PIECE_SIZE; x++) {
                rotated[x][PIECE_SIZE-1-y] = shape[y][x];
            }
        }
        shape = rotated;
    }
    return shape;
}

void TetrisBoard::checkGameOver() {
    // Game is over if we can't place a new piece
    if (checkCollision()) {
        gameOver = true;
        timer->stop();
    }
}

void TetrisBoard::startNewGame() {
    board = std::vector<std::vector<int>>(BOARD_HEIGHT, std::vector<int>(BOARD_WIDTH, 0));
    score = 0;
    level = 1;
    linesCleared = 0;
    gameOver = false;
    hasSavedPiece = false;
    timer->start(500);
    initPiece(rand() % PIECES.size());
    currentCombo.remainingTicks = 0;  // Reset any active combo display
    update();
}

void TetrisBoard::hardDrop() {
    // Keep moving down until collision
    while (moveCurrentPiece(0, 1)) {}
    lockPiece();
    update();
}

void TetrisBoard::swapPiece() {
    if (!hasSavedPiece) {
        // First time saving - store current piece and spawn new one
        savedPiece = {0, 0, currentPiece.type, 0, std::vector<std::vector<bool>>(PIECE_SIZE, std::vector<bool>(PIECE_SIZE))};
        savedPiece.shape = getRotatedShape(savedPiece.type, 0);
        hasSavedPiece = true;
        initPiece(rand() % PIECES.size());
    } else {
        // Exchange current and saved pieces, resetting position and rotation
        int savedType = savedPiece.type;
        savedPiece.type = currentPiece.type;
        savedPiece.shape = getRotatedShape(savedPiece.type, 0);
        
        currentPiece.x = BOARD_WIDTH / 2 - 2;
        currentPiece.y = 0;
        currentPiece.type = savedType;
        currentPiece.rotation = 0;
        currentPiece.shape = getRotatedShape(savedType, 0);
    }
}

// Update sizeHint to accommodate score display
QSize TetrisBoard::sizeHint() const { 
    return QSize(BOARD_WIDTH * BLOCK_SIZE + 120, BOARD_HEIGHT * BLOCK_SIZE); 
}
