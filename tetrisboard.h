#ifndef TETRISBOARD_H
#define TETRISBOARD_H

#include <QWidget>
#include <QTimer>
#include <QKeyEvent>
#include <QPainter>
#include <vector>
#include <QColor>
#include <QString>

class TetrisBoard : public QWidget {
    Q_OBJECT

public:
    TetrisBoard(QWidget *parent = nullptr);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void updateGame();

private:
    static const int BOARD_WIDTH = 10;
    static const int BOARD_HEIGHT = 20;
    static const int BLOCK_SIZE = 30;
    static const int PIECE_SIZE = 4;  // Maximum size of any piece

    QTimer *timer;
    std::vector<std::vector<int>> board;  // 0 = empty, 1-7 = colors
    
    struct Piece {
        int x, y;           // Position
        int type;          // Shape type (0-6)
        int rotation;      // Current rotation (0-3)
        std::vector<std::vector<bool>> shape;  // Current rotated shape
    };
    
    static const std::vector<std::vector<std::vector<bool>>> PIECES;  // All piece definitions
    static const std::vector<QColor> COLORS;  // Colors for each piece type
    
    Piece currentPiece;
    Piece savedPiece;
    bool hasSavedPiece = false;
    void initPiece(int type);
    std::vector<std::vector<bool>> getRotatedShape(int type, int rotation);
    bool moveCurrentPiece(int dx, int dy);
    void rotatePiece();
    bool checkCollision();
    void lockPiece();
    void clearLines();

    int score = 0;
    int level = 1;
    int linesCleared = 0;
    
    void updateScore(int lines);
    void drawScore(QPainter& painter);

    bool gameOver = false;
    void checkGameOver();
    void startNewGame();

    void hardDrop();
    void swapPiece();

    struct ComboDisplay {
        QString text;
        QColor color;
        int remainingTicks;  // How many more frames to show
        QPoint position;     // Where to show on screen
    };
    
    ComboDisplay currentCombo;
    static const int COMBO_DISPLAY_TIME = 30;  // Frames to show combo
};

#endif // TETRISBOARD_H
