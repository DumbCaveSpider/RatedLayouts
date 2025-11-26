#include "RLCreatorLayer.hpp"

#include <algorithm>
#include <deque>
#include <random>

#include "LevelSearchLayer.cpp"
#include "RLLeaderboardLayer.hpp"

bool RLCreatorLayer::init() {
      if (!CCLayer::init())
            return false;

      auto winSize = CCDirector::sharedDirector()->getWinSize();

      addSideArt(this, SideArt::All, SideArtStyle::LayerGray, false);

      auto backMenu = CCMenu::create();
      backMenu->setPosition({0, 0});

      auto backButtonSpr =
          CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
      auto backButton = CCMenuItemSpriteExtra::create(
          backButtonSpr, this, menu_selector(RLCreatorLayer::onBackButton));
      backButton->setPosition({25, winSize.height - 25});
      backMenu->addChild(backButton);
      this->addChild(backMenu);

      auto mainMenu = CCMenu::create();
      mainMenu->setPosition({winSize.width / 2, winSize.height / 2});
      mainMenu->setContentSize({300.f, 240.f});
      mainMenu->setLayout(RowLayout::create()
                              ->setGap(6.f)
                              ->setGrowCrossAxis(true)
                              ->setCrossAxisOverflow(false));

      this->addChild(mainMenu);

      auto featuredSpr = CCSprite::create("RL_featuredBtn.png"_spr);
      if (!featuredSpr) featuredSpr = CCSprite::create("RL_featuredBtn.png"_spr);
      auto featuredItem = CCMenuItemSpriteExtra::create(
          featuredSpr, this, menu_selector(RLCreatorLayer::onFeaturedLayouts));
      featuredItem->setID("featured-button");
      mainMenu->addChild(featuredItem);

      auto leaderboardSpr = CCSprite::create("RL_leaderboardBtn.png"_spr);
      if (!leaderboardSpr) leaderboardSpr = CCSprite::create("RL_leaderboardBtn.png"_spr);
      auto leaderboardItem = CCMenuItemSpriteExtra::create(
          leaderboardSpr, this, menu_selector(RLCreatorLayer::onLeaderboard));
      leaderboardItem->setID("leaderboard-button");
      mainMenu->addChild(leaderboardItem);

      auto newlySpr = CCSprite::create("RL_newRatedBtn.png"_spr);
      if (!newlySpr) newlySpr = CCSprite::create("RL_newRatedBtn.png"_spr);
      auto newlyItem = CCMenuItemSpriteExtra::create(
          newlySpr, this, menu_selector(RLCreatorLayer::onNewRated));
      newlyItem->setID("newly-rated-button");
      mainMenu->addChild(newlyItem);

      auto sendSpr = CCSprite::create("RL_sendLayoutsBtn.png"_spr);
      if (!sendSpr) sendSpr = CCSprite::create("RL_sendLayoutsBtn.png"_spr);
      auto sendItem = CCMenuItemSpriteExtra::create(
          sendSpr, this, menu_selector(RLCreatorLayer::onSendLayouts));
      sendItem->setID("send-layouts-button");
      mainMenu->addChild(sendItem);
      mainMenu->updateLayout();

      auto mainMenuBg = CCScale9Sprite::create("square02_001.png");
      mainMenuBg->setContentSize(mainMenu->getContentSize());
      mainMenuBg->setAnchorPoint({0, 0});
      mainMenuBg->setOpacity(150);
      mainMenu->addChild(mainMenuBg, -1);

      // test the ground moving thingy :o
      // idk how gd actually does it correctly but this is close enough i guess
      m_bgContainer = CCNode::create();
      m_bgContainer->setContentSize(winSize);
      this->addChild(m_bgContainer, -4);

      std::string bgName = "game_bg_01_001.png";
      auto testBg = CCSprite::create(bgName.c_str());
      if (!testBg) {
            testBg = CCSprite::create("game_bg_01_001.png");
      }
      if (testBg) {
            float tileW = testBg->getContentSize().width;
            int tiles = static_cast<int>(ceil(winSize.width / tileW)) + 2;
            for (int i = 0; i < tiles; ++i) {
                  auto bgSpr = CCSprite::create(bgName.c_str());
                  if (!bgSpr) bgSpr = CCSprite::create("game_bg_01_001.png");
                  if (!bgSpr) continue;
                  bgSpr->setAnchorPoint({0.f, 0.f});
                  bgSpr->setPosition({i * tileW, 0.f});
                  bgSpr->setColor({40, 125, 255});
                  m_bgContainer->addChild(bgSpr);
                  m_bgTiles.push_back(bgSpr);
            }
      }

      m_groundContainer = CCNode::create();
      m_groundContainer->setContentSize(winSize);
      this->addChild(m_groundContainer, -3);

      std::string groundName = "groundSquare_01_001.png";
      auto testGround = CCSprite::create(groundName.c_str());
      if (!testGround) testGround = CCSprite::create("groundSquare_01_001.png");
      if (testGround) {
            float tileW = testGround->getContentSize().width;
            int tiles = static_cast<int>(ceil(winSize.width / tileW)) + 2;
            for (int i = 0; i < tiles; ++i) {
                  auto gSpr = CCSprite::create(groundName.c_str());
                  if (!gSpr) gSpr = CCSprite::create("groundSquare_01_001.png");
                  if (!gSpr) continue;
                  gSpr->setAnchorPoint({0.f, 0.f});
                  gSpr->setPosition({i * tileW, -70.f});
                  gSpr->setColor({0, 102, 255});
                  m_groundContainer->addChild(gSpr);
                  m_groundTiles.push_back(gSpr);
            }
      }

      // spawn random decorations behind the ground
      // this gonna hurt my brain, fk math
      {
            std::pair<const char*, const char*> squarePair = {"square_01_001.png", "square_01_001.png"};
            std::vector<std::pair<const char*, const char*>> spikeNames = {
                {"spike_01_001.png", "spike_01_001.png"},
                {"spike_02_001.png", "spike_02_001.png"},
                {"spike_03_001.png", "spike_03_001.png"}};

            // determine the max decoration sprite footprint
            // only consider squares & spikes
            float maxW = 0.f;
            float maxH = 0.f;
            // include square & spike sizes in max footprint
            float squareW = 0.f;
            float squareH = 0.f;
            {
                  auto sq = CCSprite::createWithSpriteFrameName(squarePair.first);
                  if (!sq) sq = CCSprite::create(squarePair.second);
                  if (sq) {
                        squareW = sq->getContentSize().width;
                        squareH = sq->getContentSize().height;
                        maxW = std::max(maxW, squareW);
                        maxH = std::max(maxH, squareH);
                  }
            }
            for (auto& p : spikeNames) {
                  auto spr = CCSprite::createWithSpriteFrameName(p.first);
                  if (!spr) spr = CCSprite::create(p.second);
                  if (!spr) continue;
                  maxW = std::max(maxW, spr->getContentSize().width);
                  maxH = std::max(maxH, spr->getContentSize().height);
            }
            // Prefer square footprint for grid sizing so squares touch each other without gaps
            float gridX = std::max(30.f, squareW == 0.f ? maxW : squareW);
            float gridY = std::max(30.f, squareH == 0.f ? maxH : squareH);
            int cols = static_cast<int>(ceil(winSize.width / gridX)) + 2;
            // determine vertical range: above ground top to near top of screen
            float groundH = testGround ? testGround->getContentSize().height : 0.f;
            float groundY = m_groundTiles.size() ? m_groundTiles.front()->getPositionY() : -70.f;
            float minY = groundY + groundH;  // a bit above the ground
            float maxY = winSize.height;
            int rows = std::max(1, static_cast<int>((maxY - minY) / gridY));
            m_decoGridX = gridX;
            m_decoGridY = gridY;
            m_decoCols = cols;
            m_decoRows = rows;

            // occupancy board to avoid overlapping decorations
            std::vector<std::vector<bool>> occupied(rows, std::vector<bool>(cols, false));
            // square tracking to ensure connectivity and for spikes
            std::vector<std::vector<bool>> squareGrid(rows, std::vector<bool>(cols, false));
            struct SquareEntry {
                  int row;
                  int col;
                  CCSprite* spr;
                  float w;
                  float h;
                  int spanCols;
                  int spanRows;
            };
            std::vector<SquareEntry> squares;

            std::random_device rd;  // me when i do random.randint() like in python but this is crappy c++. ahh
            std::mt19937 gen(rd());
            std::uniform_real_distribution<float> xOffsetDist(0.0f, 0.0f);
            std::uniform_real_distribution<float> yOffsetDist(0.0f, 0.0f);
            // keep square frequency higher through direct square pass + higher spawn chance

            // probabilities & tuning - modify these to change density/cluster sizes
            float spawnChanceSquare = 0.10f;  // chance to start a square cluster on a ground cell
            float clusterGrowChance = 0.85f;  // chance to expand a cluster into neighbors
            int maxClusterSize = 8;           // max size of clusters
            int maxObjects = 60;              // global spawn cap
            int objectsSpawned = 0;

            // Square placement pass with cluster growth
            std::uniform_real_distribution<float> p(0.f, 1.f);
            for (int row = 0; row < rows; ++row) {
                  if (objectsSpawned >= maxObjects) break;
                  for (int col = 0; col < cols; ++col) {
                        if (occupied[row][col]) continue;
                        if (objectsSpawned >= maxObjects) break;
                        // Only attempt to start clusters at the ground row (row == 0) to ensure connectivity
                        if (row != 0) continue;
                        if (p(gen) > spawnChanceSquare) continue;
                        // place initial square here
                        auto sq = CCSprite::createWithSpriteFrameName(squarePair.first);
                        if (!sq) sq = CCSprite::create(squarePair.second);
                        if (!sq) continue;
                        // anchor & pos
                        sq->setAnchorPoint({0.f, 0.f});
                        float gx = col * gridX;
                        float gy = minY + row * gridY;
                        // determine spans
                        float sw = sq->getContentSize().width;
                        float sh = sq->getContentSize().height;
                        int sSpanCols = std::max(1, static_cast<int>(ceil(sw / gridX)));
                        int sSpanRows = std::max(1, static_cast<int>(ceil(sh / gridY)));
                        // ensure fits
                        if (col + sSpanCols > cols || row + sSpanRows > rows) continue;
                        // ensure no occupancy across span
                        bool fits = true;
                        for (int rr = row; rr < row + sSpanRows && fits; ++rr) {
                              for (int cc2 = col; cc2 < col + sSpanCols; ++cc2) {
                                    if (occupied[rr][cc2]) {
                                          fits = false;
                                          break;
                                    }
                              }
                        }
                        if (!fits) continue;
                        // add to bg
                        sq->setPosition({gx, gy});
                        m_bgContainer->addChild(sq);
                        objectsSpawned++;
                        if (objectsSpawned >= maxObjects) break;
                        m_bgDecorations.push_back(sq);
                        // mark occupied & squareGrid
                        for (int rr = row; rr < row + sSpanRows; ++rr) {
                              for (int cc2 = col; cc2 < col + sSpanCols; ++cc2) {
                                    occupied[rr][cc2] = true;
                                    squareGrid[rr][cc2] = true;
                              }
                        }
                        squares.push_back(SquareEntry{row, col, sq, sw, sh, sSpanCols, sSpanRows});
                        // cluster growth BFS-style
                        std::deque<std::pair<int, int>> q;
                        q.push_back({row, col});
                        int clusterCount = 1;
                        // use tuned maxClusterSize
                        while (!q.empty() && clusterCount < maxClusterSize && objectsSpawned < maxObjects) {
                              auto [cr, cc] = q.front();
                              q.pop_front();
                              std::vector<std::pair<int, int>> neigh = {{cr, cc - 1}, {cr, cc + 1}, {cr - 1, cc}, {cr + 1, cc}};
                              std::shuffle(neigh.begin(), neigh.end(), gen);
                              for (auto [nr, nc] : neigh) {
                                    if (clusterCount >= maxClusterSize) break;
                                    if (nr < 0 || nr >= rows || nc < 0 || nc >= cols) continue;
                                    if (occupied[nr][nc]) continue;
                                    if (p(gen) > clusterGrowChance) continue;  // skip if growth chance not met
                                    // place a new square at nr,nc
                                    auto nsq = CCSprite::createWithSpriteFrameName(squarePair.first);
                                    if (!nsq) nsq = CCSprite::create(squarePair.second);
                                    if (!nsq) continue;
                                    nsq->setAnchorPoint({0.f, 0.f});
                                    float ngx = nc * gridX;
                                    float ngy = minY + nr * gridY;
                                    float nsw = nsq->getContentSize().width;
                                    float nsh = nsq->getContentSize().height;
                                    int nSpanCols = std::max(1, static_cast<int>(ceil(nsw / gridX)));
                                    int nSpanRows = std::max(1, static_cast<int>(ceil(nsh / gridY)));
                                    if (nc + nSpanCols > cols || nr + nSpanRows > rows) continue;
                                    bool nfits = true;
                                    for (int rr = nr; rr < nr + nSpanRows && nfits; ++rr) {
                                          for (int cc2 = nc; cc2 < nc + nSpanCols; ++cc2) {
                                                if (occupied[rr][cc2]) {
                                                      nfits = false;
                                                      break;
                                                }
                                          }
                                    }
                                    if (!nfits) continue;
                                    nsq->setPosition({ngx, ngy});
                                    m_bgContainer->addChild(nsq);
                                    objectsSpawned++;
                                    if (objectsSpawned >= maxObjects) break;
                                    m_bgDecorations.push_back(nsq);
                                    for (int rr = nr; rr < nr + nSpanRows; ++rr) {
                                          for (int cc2 = nc; cc2 < nc + nSpanCols; ++cc2) {
                                                occupied[rr][cc2] = true;
                                                squareGrid[rr][cc2] = true;
                                          }
                                    }
                                    squares.push_back(SquareEntry{nr, nc, nsq, nsw, nsh, nSpanCols, nSpanRows});
                                    q.push_back({nr, nc});
                                    clusterCount++;
                              }
                        }
                  }
            }

            // any isolated squares will try to be connected by forcing a neighbor
            auto hasNeighbor = [&](const SquareEntry& sq) -> bool {
                  int r = sq.row;
                  int c = sq.col;
                  bool l = false, rgt = false, up = false, down = false;
                  int leftCol = c - 1;
                  int rightCol = c + sq.spanCols;
                  int topRow = r - 1;
                  int bottomRow = r + sq.spanRows;
                  if (leftCol >= 0) {
                        for (int rr = r; rr < r + sq.spanRows && !l; ++rr) {
                              if (squareGrid[rr][leftCol]) l = true;
                        }
                  }
                  if (rightCol < cols) {
                        for (int rr = r; rr < r + sq.spanRows && !rgt; ++rr) {
                              if (squareGrid[rr][rightCol]) rgt = true;
                        }
                  }
                  if (topRow >= 0) {
                        for (int cc2 = c; cc2 < c + sq.spanCols && !up; ++cc2) {
                              if (squareGrid[topRow][cc2]) up = true;
                        }
                  }
                  if (bottomRow < rows) {
                        for (int cc2 = c; cc2 < c + sq.spanCols && !down; ++cc2) {
                              if (squareGrid[bottomRow][cc2]) down = true;
                        }
                  }
                  return l || rgt || up || down;
            };

            for (size_t i = 0; i < squares.size(); ++i) {
                  if (objectsSpawned >= maxObjects) break;
                  auto& sq = squares[i];
                  if (hasNeighbor(sq)) continue;
                  // try to force create a neighbor in prioritized directions
                  std::vector<std::pair<int, int>> dirs = {{sq.row, sq.col - 1}, {sq.row, sq.col + 1}, {sq.row - 1, sq.col}, {sq.row + 1, sq.col}};
                  for (auto [nr, nc] : dirs) {
                        if (nr < 0 || nr >= rows || nc < 0 || nc >= cols) continue;
                        if (occupied[nr][nc]) continue;
                        // create forced square there
                        auto nsq = CCSprite::createWithSpriteFrameName(squarePair.first);
                        if (!nsq) nsq = CCSprite::create(squarePair.second);
                        if (!nsq) continue;
                        float ngx = nc * gridX;
                        float ngy = minY + nr * gridY;
                        nsq->setAnchorPoint({0.f, 0.f});
                        nsq->setPosition({ngx, ngy});
                        m_bgContainer->addChild(nsq);
                        objectsSpawned++;
                        if (objectsSpawned >= maxObjects) break;
                        m_bgDecorations.push_back(nsq);
                        float nsw = nsq->getContentSize().width;
                        float nsh = nsq->getContentSize().height;
                        int nSpanCols = std::max(1, static_cast<int>(ceil(nsw / gridX)));
                        int nSpanRows = std::max(1, static_cast<int>(ceil(nsh / gridY)));
                        for (int rr = nr; rr < nr + nSpanRows && rr < rows; ++rr) {
                              for (int cc2 = nc; cc2 < nc + nSpanCols && cc2 < cols; ++cc2) {
                                    occupied[rr][cc2] = true;
                                    squareGrid[rr][cc2] = true;
                              }
                        }
                        squares.push_back(SquareEntry{nr, nc, nsq, nsw, nsh, nSpanCols, nSpanRows});
                        break;  // break after placing one forced neighbor
                  }
            }

            // ensure squares are connected: if a square has no left/right square, try to place a neighbor
            for (size_t si = 0; si < squares.size(); ++si) {
                  if (objectsSpawned >= maxObjects) break;
                  auto& sq = squares[si];
                  int r = sq.row;
                  int c = sq.col;
                  // check adjacency on full spans
                  bool left = false;
                  bool right = false;
                  bool up = false;
                  bool down = false;
                  int leftCol = c - 1;
                  int rightCol = c + sq.spanCols;
                  int topRow = r - 1;
                  int bottomRow = r + sq.spanRows;
                  // left
                  if (leftCol >= 0) {
                        for (int rr = r; rr < r + sq.spanRows && !left; ++rr) {
                              if (squareGrid[rr][leftCol]) left = true;
                        }
                  }
                  // right
                  if (rightCol < cols) {
                        for (int rr = r; rr < r + sq.spanRows && !right; ++rr) {
                              if (squareGrid[rr][rightCol]) right = true;
                        }
                  }
                  // up
                  if (topRow >= 0) {
                        for (int cc2 = c; cc2 < c + sq.spanCols && !up; ++cc2) {
                              if (squareGrid[topRow][cc2]) up = true;
                        }
                  }
                  // down
                  if (bottomRow < rows) {
                        for (int cc2 = c; cc2 < c + sq.spanCols && !down; ++cc2) {
                              if (squareGrid[bottomRow][cc2]) down = true;
                        }
                  }
                  // if square has no horizontal neighbor, try to connect it
                  if (!left && !right && !up && !down) {
                        std::uniform_real_distribution<float> connectChance(0.f, 1.f);
                        // always attempt (high chance) to add at least one neighbor
                        if (connectChance(gen) < 0.9f) {
                              std::uniform_int_distribution<int> dir(0, 3);
                              int d = dir(gen);  // 0=left, 1=right, 2=up, 3=down
                              int nc = c;
                              int nr = r;
                              if (d == 0)
                                    nc = c - 1;
                              else if (d == 1)
                                    nc = c + 1;
                              else if (d == 2)
                                    nr = r - 1;
                              else if (d == 3)
                                    nr = r + 1;
                              // check bounds and occupancy for chosen direction
                              if (nr < 0 || nr >= rows || nc < 0 || nc >= cols) {
                                    continue;
                              }
                              // don't attempt if occupied
                              if (occupied[nr][nc]) continue;
                              if (true) {
                                    // create a new square at (r, nc)
                                    auto newSq = CCSprite::createWithSpriteFrameName("square_01_001.png");
                                    if (!newSq) newSq = CCSprite::create("square_01_001.png");
                                    if (newSq) {
                                          if (objectsSpawned >= maxObjects) break;
                                          newSq->setAnchorPoint({0.f, 0.f});
                                          float gx2 = nc * gridX;
                                          float gy2 = minY + nr * gridY;
                                          newSq->setPosition({gx2, gy2});
                                          m_bgContainer->addChild(newSq);
                                          objectsSpawned++;
                                          if (objectsSpawned >= maxObjects) break;
                                          m_bgDecorations.push_back(newSq);
                                          float w2 = newSq->getContentSize().width;
                                          float h2 = newSq->getContentSize().height;
                                          int spanCols2 = std::max(1, static_cast<int>(ceil(w2 / gridX)));
                                          int spanRows2 = std::max(1, static_cast<int>(ceil(h2 / gridY)));
                                          for (int rr = nr; rr < nr + spanRows2 && rr < rows; ++rr) {
                                                for (int cc2 = nc; cc2 < nc + spanCols2 && cc2 < cols; ++cc2) {
                                                      occupied[rr][cc2] = true;
                                                      squareGrid[rr][cc2] = true;
                                                }
                                          }
                                          SquareEntry e2{r, nc, newSq, w2, h2, spanCols2, spanRows2};
                                          squares.push_back(e2);
                                    }
                              }
                        }
                  }
                  // Random growth: try to add extra neighbors around squares randomly
                  std::uniform_real_distribution<float> growChance(0.f, 1.f);
                  if (growChance(gen) < 0.25f) {
                        // try left
                        int nc = c - 1;
                        if (nc >= 0 && !occupied[r][nc]) {
                              auto newSq = CCSprite::createWithSpriteFrameName("square_01_001.png");
                              if (!newSq) newSq = CCSprite::create("square_01_001.png");
                              if (newSq) {
                                    if (objectsSpawned >= maxObjects) break;
                                    newSq->setAnchorPoint({0.f, 0.f});
                                    float gx2 = nc * gridX;
                                    float gy2 = minY + r * gridY;
                                    newSq->setPosition({gx2, gy2});
                                    m_bgContainer->addChild(newSq);
                                    objectsSpawned++;
                                    if (objectsSpawned >= maxObjects) break;
                                    m_bgDecorations.push_back(newSq);
                                    float w2 = newSq->getContentSize().width;
                                    float h2 = newSq->getContentSize().height;
                                    int spanCols2 = std::max(1, static_cast<int>(ceil(w2 / gridX)));
                                    int spanRows2 = std::max(1, static_cast<int>(ceil(h2 / gridY)));
                                    for (int rr = r; rr < r + spanRows2 && rr < rows; ++rr) {
                                          for (int cc2 = nc; cc2 < nc + spanCols2 && cc2 < cols; ++cc2) {
                                                occupied[rr][cc2] = true;
                                                squareGrid[rr][cc2] = true;
                                          }
                                    }
                                    SquareEntry e3{r, nc, newSq, w2, h2, spanCols2, spanRows2};
                                    squares.push_back(e3);
                              }
                        }
                  }
                  if (growChance(gen) < 0.25f) {
                        // try right
                        int nc = c + 1;
                        if (nc < cols && !occupied[r][nc]) {
                              auto newSq = CCSprite::createWithSpriteFrameName("square_01_001.png");
                              if (!newSq) newSq = CCSprite::create("square_01_001.png");
                              if (newSq) {
                                    if (objectsSpawned >= maxObjects) break;
                                    newSq->setAnchorPoint({0.f, 0.f});
                                    float gx2 = nc * gridX;
                                    float gy2 = minY + r * gridY;
                                    newSq->setPosition({gx2, gy2});
                                    m_bgContainer->addChild(newSq);
                                    objectsSpawned++;
                                    if (objectsSpawned >= maxObjects) break;
                                    m_bgDecorations.push_back(newSq);
                                    float w2 = newSq->getContentSize().width;
                                    float h2 = newSq->getContentSize().height;
                                    int spanCols2 = std::max(1, static_cast<int>(ceil(w2 / gridX)));
                                    int spanRows2 = std::max(1, static_cast<int>(ceil(h2 / gridY)));
                                    for (int rr = r; rr < r + spanRows2 && rr < rows; ++rr) {
                                          for (int cc2 = nc; cc2 < nc + spanCols2 && cc2 < cols; ++cc2) {
                                                occupied[rr][cc2] = true;
                                                squareGrid[rr][cc2] = true;
                                          }
                                    }
                                    SquareEntry e4{r, nc, newSq, w2, h2, spanCols2, spanRows2};
                                    squares.push_back(e4);
                              }
                        }
                  }
            }

            // spawn spikes on top of squares (or on ground if no square present) and ensure spikes are not floating
            if (objectsSpawned < maxObjects) {
                  // spike tuning
                  float spikeChancePerSquare = 0.40f;  // 40% of squares get a spike
                  std::uniform_real_distribution<float> spikeChance(0.f, 1.f);
                  std::discrete_distribution<int> spikePick({1, 1, 1});
                  // spawn spike on squares
                  for (auto& sq : squares) {
                        if (objectsSpawned >= maxObjects) break;
                        if (spikeChance(gen) > spikeChancePerSquare) continue;
                        // don't spawn a spike if this square is stacked on another square (on top)
                        bool isStacked = false;
                        if (sq.row > 0) {
                              for (int cc2 = sq.col; cc2 < sq.col + sq.spanCols; ++cc2) {
                                    if (squareGrid[sq.row - 1][cc2]) {
                                          isStacked = true;
                                          break;
                                    }
                              }
                        }
                        if (isStacked) continue;
                        auto spikePair = spikeNames[spikePick(gen)];
                        const char* spFrame = spikePair.first;
                        const char* spFile = spikePair.second;
                        auto spSpr = CCSprite::createWithSpriteFrameName(spFrame);
                        if (!spSpr) spSpr = CCSprite::create(spFile);
                        if (!spSpr) continue;
                        spSpr->setAnchorPoint({0.f, 0.f});
                        // place spike on top of square; center it on the square
                        // compute spike grid placement based on the base square grid position to avoid rounding issues
                        float spW = spSpr->getContentSize().width;
                        float spH = spSpr->getContentSize().height;
                        int spSpanCols = std::max(1, static_cast<int>(ceil(spW / gridX)));
                        int spSpanRows = std::max(1, static_cast<int>(ceil(spH / gridY)));
                        // center the spike horizontally within the square span
                        if (spSpanCols > sq.spanCols) continue;  // cannot place spike if it's wider than the square
                        int baseCol = sq.col + (sq.spanCols - spSpanCols) / 2;
                        int baseRow = sq.row + sq.spanRows;  // the spike sits right above the square rows
                        float sx = baseCol * gridX + std::max(0.f, (sq.w - spW) / 2.f);
                        float sy = minY + baseRow * gridY;  // sit on top of the square row
                        int spCol = baseCol;
                        int spRow = baseRow;
                        bool canPlace = true;
                        for (int rr = spRow; rr < spRow + spSpanRows && canPlace; ++rr) {
                              for (int cc2 = spCol; cc2 < spCol + spSpanCols; ++cc2) {
                                    if (rr < 0 || rr >= rows || cc2 < 0 || cc2 >= cols) {
                                          canPlace = false;
                                          break;
                                    }
                                    // if occupied and that occupied cell isn't part of any square from this spike's base, skip
                                    if (occupied[rr][cc2]) {
                                          if (!squareGrid[rr][cc2]) {
                                                canPlace = false;
                                                break;
                                          }
                                          // if squareGrid is true, ensure it's part of this square (i.e., within sq span)
                                          bool inBaseSquare = false;
                                          for (int br = sq.row; br < sq.row + sq.spanRows && !inBaseSquare; ++br) {
                                                for (int bc = sq.col; bc < sq.col + sq.spanCols; ++bc) {
                                                      if (br == rr && bc == cc2) {
                                                            inBaseSquare = true;
                                                            break;
                                                      }
                                                }
                                          }
                                          if (!inBaseSquare) {
                                                canPlace = false;
                                                break;
                                          }
                                    }
                              }
                        }
                        // Additional pixel-bounding-box check: make sure the spike will not overlap any other square sprite's rect
                        if (canPlace) {
                              float spikeL = sx;
                              float spikeR = sx + spW;
                              float spikeB = sy;
                              float spikeT = sy + spH;
                              for (auto& other : squares) {
                                    if (other.spr == sq.spr) continue;  // skip base square
                                    float ol = other.spr->getPositionX();
                                    float ob = other.spr->getPositionY();
                                    float oright = ol + other.w;
                                    float otop = ob + other.h;
                                    bool intersect = !(spikeR <= ol || spikeL >= oright || spikeT <= ob || spikeB >= otop);
                                    if (intersect) {
                                          canPlace = false;
                                          break;
                                    }
                              }
                        }
                        if (!canPlace) continue;
                        // add to ground container so it's visible on top
                        m_groundContainer->addChild(spSpr, 1);
                        objectsSpawned++;
                        if (objectsSpawned >= maxObjects) break;
                        spSpr->setPosition({sx, sy});
                        m_bgDecorations.push_back(spSpr);
                        // mark occupancy cells for spike so no future deco spawns on it
                        for (int rr = spRow; rr < spRow + spSpanRows && rr < rows; ++rr) {
                              for (int cc2 = spCol; cc2 < spCol + spSpanCols && cc2 < cols; ++cc2) {
                                    occupied[rr][cc2] = true;
                              }
                        }
                  }
                  // Prioritize placing squares first, then ground spikes
                  if (objectsSpawned < maxObjects) {
                        // gather all empty cells
                        std::vector<std::pair<int, int>> empties;
                        empties.reserve(rows * cols);
                        for (int rr = 0; rr < rows; ++rr) {
                              for (int cc2 = 0; cc2 < cols; ++cc2) {
                                    if (!occupied[rr][cc2]) empties.push_back({rr, cc2});
                              }
                        }
                        // try to place squares in empty cells, preferring ground-adjacent cells first
                        // sort empties by priority: cells adjacent to squareGrid cells get higher priority
                        std::sort(empties.begin(), empties.end(), [&](const auto& a, const auto& b) {
                              auto score = [&](int r, int c) {
                                    int s = 0;
                                    for (int dr = -1; dr <= 1; ++dr) {
                                          for (int dc = -1; dc <= 1; ++dc) {
                                                if (dr == 0 && dc == 0) continue;
                                                int nr = r + dr;
                                                int nc = c + dc;
                                                if (nr >= 0 && nr < rows && nc >= 0 && nc < cols && squareGrid[nr][nc]) s += 2;
                                          }
                                    }
                                    // ground cells get a small boost
                                    if (r == 0) s += 1;
                                    return s;
                              };
                              return score(a.first, a.second) > score(b.first, b.second);
                        });

                        for (auto [rr, cc2] : empties) {
                              if (objectsSpawned >= maxObjects) break;
                              // ensure still empty
                              if (occupied[rr][cc2]) continue;
                              // try to place a square
                              auto fillSq = CCSprite::createWithSpriteFrameName(squarePair.first);
                              if (!fillSq) fillSq = CCSprite::create(squarePair.second);
                              if (!fillSq) continue;
                              float fw = fillSq->getContentSize().width;
                              float fh = fillSq->getContentSize().height;
                              int fSpanCols = std::max(1, static_cast<int>(ceil(fw / gridX)));
                              int fSpanRows = std::max(1, static_cast<int>(ceil(fh / gridY)));
                              if (cc2 + fSpanCols > cols || rr + fSpanRows > rows) continue;
                              bool ffits = true;
                              for (int r2 = rr; r2 < rr + fSpanRows && ffits; ++r2) {
                                    for (int c2 = cc2; c2 < cc2 + fSpanCols; ++c2) {
                                          if (occupied[r2][c2]) {
                                                ffits = false;
                                                break;
                                          }
                                    }
                              }
                              if (!ffits) continue;
                              // place square
                              fillSq->setAnchorPoint({0.f, 0.f});
                              float gx2 = cc2 * gridX;
                              float gy2 = minY + rr * gridY;
                              fillSq->setPosition({gx2, gy2});
                              m_bgContainer->addChild(fillSq);
                              objectsSpawned++;
                              m_bgDecorations.push_back(fillSq);
                              for (int r2 = rr; r2 < rr + fSpanRows && r2 < rows; ++r2) {
                                    for (int c2 = cc2; c2 < cc2 + fSpanCols && c2 < cols; ++c2) {
                                          occupied[r2][c2] = true;
                                          squareGrid[r2][c2] = true;
                                    }
                              }
                              squares.push_back(SquareEntry{rr, cc2, fillSq, fw, fh, fSpanCols, fSpanRows});
                        }
                        // if still short, try to place ground spikes on empty ground cells
                        if (objectsSpawned < maxObjects) {
                              for (int col2 = 0; col2 < cols; ++col2) {
                                    if (objectsSpawned >= maxObjects) break;
                                    if (occupied[0][col2]) continue;
                                    auto spikePair = spikeNames[spikePick(gen)];
                                    const char* spFrame = spikePair.first;
                                    const char* spFile = spikePair.second;
                                    auto spSpr = CCSprite::createWithSpriteFrameName(spFrame);
                                    if (!spSpr) spSpr = CCSprite::create(spFile);
                                    if (!spSpr) continue;
                                    spSpr->setAnchorPoint({0.f, 0.f});
                                    float sx = col2 * gridX;
                                    float sy = minY;  // on ground
                                    float spW = spSpr->getContentSize().width;
                                    sx += std::max(0.f, (gridX - spW) / 2.f);
                                    int spSpanCols = std::max(1, static_cast<int>(ceil(spW / gridX)));
                                    int spSpanRows = std::max(1, static_cast<int>(ceil(spSpr->getContentSize().height / gridY)));
                                    int spCol = static_cast<int>(floor((sx) / gridX));
                                    int spRow = static_cast<int>(floor((sy - minY) / gridY));
                                    bool canPlace = true;
                                    for (int r2 = spRow; r2 < spRow + spSpanRows && canPlace; ++r2) {
                                          for (int c2 = spCol; c2 < spCol + spSpanCols; ++c2) {
                                                if (r2 < 0 || r2 >= rows || c2 < 0 || c2 >= cols) {
                                                      canPlace = false;
                                                      break;
                                                }
                                                if (occupied[r2][c2]) {
                                                      canPlace = false;
                                                      break;
                                                }
                                          }
                                    }
                                    if (!canPlace) continue;
                                    m_groundContainer->addChild(spSpr, 1);
                                    objectsSpawned++;
                                    spSpr->setPosition({sx, sy});
                                    m_bgDecorations.push_back(spSpr);
                                    for (int r2 = spRow; r2 < spRow + spSpanRows && r2 < rows; ++r2) {
                                          for (int c2 = spCol; c2 < spCol + spSpanCols && c2 < cols; ++c2) {
                                                occupied[r2][c2] = true;
                                          }
                                    }
                              }
                        }
                  }
                  // optionally spawn some ground spikes in empty columns
                  float groundSpikeChance = 0.08f;  // 8% chance per column
                  for (int col = 0; col < cols; ++col) {
                        if (objectsSpawned >= maxObjects) break;
                        if (spikeChance(gen) > groundSpikeChance) continue;
                        if (occupied[0][col]) continue;  // leave a bit of ground empty
                        auto spikePair = spikeNames[spikePick(gen)];
                        const char* spFrame = spikePair.first;
                        const char* spFile = spikePair.second;
                        auto spSpr = CCSprite::createWithSpriteFrameName(spFrame);
                        if (!spSpr) spSpr = CCSprite::create(spFile);
                        if (!spSpr) continue;
                        spSpr->setAnchorPoint({0.f, 0.f});
                        float sx = col * gridX;
                        float sy = minY;  // on ground
                        // center spike on grid cell
                        float spW = spSpr->getContentSize().width;
                        sx += std::max(0.f, (gridX - spW) / 2.f);
                        int spSpanCols = std::max(1, static_cast<int>(ceil(spW / gridX)));
                        int spSpanRows = std::max(1, static_cast<int>(ceil(spSpr->getContentSize().height / gridY)));
                        int spCol = static_cast<int>(floor((sx) / gridX));
                        int spRow = static_cast<int>(floor((sy - minY) / gridY));
                        bool canPlace = true;
                        for (int rr = spRow; rr < spRow + spSpanRows && canPlace; ++rr) {
                              for (int cc2 = spCol; cc2 < spCol + spSpanCols; ++cc2) {
                                    if (rr < 0 || rr >= rows || cc2 < 0 || cc2 >= cols) {
                                          canPlace = false;
                                          break;
                                    }
                                    if (occupied[rr][cc2]) {
                                          canPlace = false;
                                          break;
                                    }
                              }
                        }
                        if (!canPlace) continue;
                        m_groundContainer->addChild(spSpr, 1);
                        objectsSpawned++;
                        if (objectsSpawned >= maxObjects) break;
                        spSpr->setPosition({sx, sy});
                        m_bgDecorations.push_back(spSpr);
                        for (int rr = spRow; rr < spRow + spSpanRows && rr < rows; ++rr) {
                              for (int cc2 = spCol; cc2 < spCol + spSpanCols && cc2 < cols; ++cc2) {
                                    occupied[rr][cc2] = true;
                              }
                        }
                  }
            }
      }

      auto floorLineSpr = CCSprite::createWithSpriteFrameName("floorLine_01_001.png");
      floorLineSpr->setPosition({winSize.width / 2, 58});
      m_groundContainer->addChild(floorLineSpr, 0);

      this->scheduleUpdate();
      this->setKeypadEnabled(true);
      return true;
}

void RLCreatorLayer::onBackButton(CCObject* sender) {
      CCDirector::sharedDirector()->popSceneWithTransition(
          0.5f, PopTransition::kPopTransitionFade);
}

void RLCreatorLayer::onLeaderboard(CCObject* sender) {
      auto leaderboardLayer = RLLeaderboardLayer::create();
      auto scene = CCScene::create();
      scene->addChild(leaderboardLayer);
      auto transitionFade = CCTransitionFade::create(0.5f, scene);
      CCDirector::sharedDirector()->pushScene(transitionFade);
}

void RLCreatorLayer::onFeaturedLayouts(CCObject* sender) {
      web::WebRequest()
          .param("type", 2)
          .param("amount", 1000)
          .get("https://gdrate.arcticwoof.xyz/getLevels")
          .listen([this](web::WebResponse* res) {
                if (res && res->ok()) {
                      auto jsonResult = res->json();

                      if (jsonResult) {
                            auto json = jsonResult.unwrap();
                            std::string levelIDs;
                            bool first = true;

                            if (json.contains("levelIds")) {
                                  auto levelsArr = json["levelIds"];

                                  // iterate
                                  for (auto levelIDValue : levelsArr) {
                                        auto levelID = levelIDValue.as<int>();
                                        if (levelID) {
                                              if (!first)
                                                    levelIDs += ",";
                                              levelIDs += numToString(levelID.unwrap());
                                              first = false;
                                        }
                                  }
                            }

                            if (!levelIDs.empty()) {
                                  auto searchObject =
                                      GJSearchObject::create(SearchType::Type19, levelIDs);
                                  auto browserLayer = LevelBrowserLayer::create(searchObject);
                                  auto scene = CCScene::create();
                                  scene->addChild(browserLayer);
                                  auto transitionFade = CCTransitionFade::create(0.5f, scene);
                                  CCDirector::sharedDirector()->pushScene(transitionFade);
                            } else {
                                  log::warn("No levels found in response");
                                  Notification::create("No featured levels found",
                                                       NotificationIcon::Warning)
                                      ->show();
                            }
                      } else {
                            log::error("Failed to parse response JSON");
                      }
                } else {
                      log::error("Failed to fetch levels from server");
                      Notification::create("Failed to fetch levels from server",
                                           NotificationIcon::Error)
                          ->show();
                }
          });
}

void RLCreatorLayer::onNewRated(CCObject* sender) {
      web::WebRequest()
          .param("type", 3)
          .param("amount", 1000)
          .get("https://gdrate.arcticwoof.xyz/getLevels")
          .listen([this](web::WebResponse* res) {
                if (res && res->ok()) {
                      auto jsonResult = res->json();

                      if (jsonResult) {
                            auto json = jsonResult.unwrap();
                            std::string levelIDs;
                            bool first = true;

                            if (json.contains("levelIds")) {
                                  auto levelsArr = json["levelIds"];

                                  // iterate
                                  for (auto levelIDValue : levelsArr) {
                                        auto levelID = levelIDValue.as<int>();
                                        if (levelID) {
                                              if (!first)
                                                    levelIDs += ",";
                                              levelIDs += numToString(levelID.unwrap());
                                              first = false;
                                        }
                                  }
                            }

                            if (!levelIDs.empty()) {
                                  auto searchObject =
                                      GJSearchObject::create(SearchType::Type19, levelIDs);
                                  auto browserLayer = LevelBrowserLayer::create(searchObject);
                                  auto scene = CCScene::create();
                                  scene->addChild(browserLayer);
                                  auto transitionFade = CCTransitionFade::create(0.5f, scene);
                                  CCDirector::sharedDirector()->pushScene(transitionFade);
                            } else {
                                  log::warn("No levels found in response");
                                  Notification::create("No levels found",
                                                       NotificationIcon::Warning)
                                      ->show();
                            }
                      } else {
                            log::error("Failed to parse response JSON");
                      }
                } else {
                      log::error("Failed to fetch levels from server");
                      Notification::create("Failed to fetch levels from server",
                                           NotificationIcon::Error)
                          ->show();
                }
          });
}

void RLCreatorLayer::onSendLayouts(CCObject* sender) {
      web::WebRequest()
          .param("type", 1)
          .param("amount", 1000)
          .get("https://gdrate.arcticwoof.xyz/getLevels")
          .listen([this](web::WebResponse* res) {
                if (res && res->ok()) {
                      auto jsonResult = res->json();

                      if (jsonResult) {
                            auto json = jsonResult.unwrap();
                            std::string levelIDs;
                            bool first = true;
                            if (json.contains("levelIds")) {
                                  auto levelsArr = json["levelIds"];

                                  // iterate
                                  for (auto levelIDValue : levelsArr) {
                                        auto levelID = levelIDValue.as<int>();
                                        if (levelID) {
                                              if (!first)
                                                    levelIDs += ",";
                                              levelIDs += numToString(levelID.unwrap());
                                              first = false;
                                        }
                                  }
                            }

                            if (!levelIDs.empty()) {
                                  auto searchObject =
                                      GJSearchObject::create(SearchType::Type19, levelIDs);
                                  auto browserLayer = LevelBrowserLayer::create(searchObject);
                                  auto scene = CCScene::create();
                                  scene->addChild(browserLayer);
                                  auto transitionFade = CCTransitionFade::create(0.5f, scene);
                                  CCDirector::sharedDirector()->pushScene(transitionFade);
                            } else {
                                  log::warn("No levels found in response");
                                  Notification::create("No send layouts found",
                                                       NotificationIcon::Warning)
                                      ->show();
                            }
                      } else {
                            log::error("Failed to parse response JSON");
                      }
                } else {
                      log::error("Failed to fetch levels from server");
                      Notification::create("Failed to fetch levels from server",
                                           NotificationIcon::Error)
                          ->show();
                }
          });
}

void RLCreatorLayer::keyBackClicked() { this->onBackButton(nullptr); }

void RLCreatorLayer::update(float dt) {
      // scroll background tiles
      if (m_bgTiles.size()) {
            float move = m_bgSpeed * dt;
            int num = static_cast<int>(m_bgTiles.size());
            for (auto spr : m_bgTiles) {
                  if (!spr)
                        continue;
                  float tileW = spr->getContentSize().width;
                  float x = spr->getPositionX();
                  x -= move;
                  if (x <= -tileW) {
                        x += tileW * num;
                  }
                  spr->setPositionX(x);
            }
      }

      // scroll ground tiles
      if (m_groundTiles.size()) {
            float move = m_groundSpeed * dt;
            int num = static_cast<int>(m_groundTiles.size());
            for (auto spr : m_groundTiles) {
                  if (!spr)
                        continue;
                  float tileW = spr->getContentSize().width;
                  float x = spr->getPositionX();
                  x -= move;
                  if (x <= -tileW) {
                        x += tileW * num;
                  }
                  spr->setPositionX(x);
            }
      }

      // move background decorations (same speed as ground)
      if (!m_bgDecorations.empty()) {
            float move = m_groundSpeed * dt;
            auto winSize = CCDirector::sharedDirector()->getWinSize();
            for (auto spr : m_bgDecorations) {
                  if (!spr) continue;
                  float w = spr->getContentSize().width;
                  float x = spr->getPositionX();
                  x -= move;
                  if (x <= -w) {
                        // wrap by columns so sprite stays in grid alignment
                        int cols = m_decoCols ? m_decoCols : static_cast<int>(ceil(winSize.width / m_decoGridX)) + 2;
                        x += cols * m_decoGridX;
                  }
                  spr->setPositionX(x);
            }
      }
}

RLCreatorLayer* RLCreatorLayer::create() {
      auto ret = new RLCreatorLayer();
      if (ret && ret->init()) {
            ret->autorelease();
            return ret;
      }
      CC_SAFE_DELETE(ret);
      return nullptr;
}
