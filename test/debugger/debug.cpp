#include "game.hpp"
#include <imgui.h>
#include "debug.hpp"
#include <sstream>

namespace Debugger
{
  void DebugEngine::draw()
  {
    static bool demoWindow{true};
    if (demoWindow)
      ImGui::ShowDemoWindow(&demoWindow);

    static bool open_fen_import{false};

    ImGui::Begin("Debugger", NULL, ImGuiWindowFlags_MenuBar);

    ImGui::BeginMenuBar();
    if (ImGui::BeginMenu("Board"))
    {
      if (ImGui::MenuItem("Reset to startpos"))
      {
        game->resetBoard();
      }
      if (ImGui::MenuItem("Import FEN"))
        open_fen_import = true;
      ImGui::EndMenu();
    }
    if (ImGui::MenuItem("Undo Move"))
    {
      game->unmakeMove();
    }
    ImGui::EndMenuBar();

    if (ImGui::CollapsingHeader("Move History"))
    {
      auto moveHistory = game->board.getMoveHistory();
      for (int i{static_cast<int>(moveHistory.size()) - 1}; i >= 0; i--)
      {
        ImGui::Text("%d: %s", i, moveHistory[i].getNotation(Chess::Piece::Empty).c_str());
      }
    }

    if (ImGui::CollapsingHeader("Move Generation"))
    {
      static bool autoMoveGen{true};
      static unsigned int moveGenDepth{1};
      static Game::PerftData data;
      static std::map<Chess::Move, unsigned long long> moves{game->perftDivide(moveGenDepth)};
      static unsigned long long totalMoves{1};
      static const unsigned int uOne = 1;
      static bool madeMove{false};

      if (madeMove)
      {
        moves = game->perftDivide(moveGenDepth);
        totalMoves = 0;
        for (auto moveset : moves)
          totalMoves += moveset.second;
        madeMove = false;
      }

      if (ImGui::Checkbox("Automatically perform movegen", &autoMoveGen))
      {
        if (autoMoveGen)
        {
          moves = game->perftDivide(moveGenDepth);
          totalMoves = 0;
          for (auto moveset : moves)
            totalMoves += moveset.second;
        }
        else if (data.running)
        {
          game->stopPerft(data);
        }
      }

      if (ImGui::InputScalar("Depth", ImGuiDataType_U32, &moveGenDepth, &uOne))
      {
        if (autoMoveGen)
        {
          moves = game->perftDivide(moveGenDepth);
          totalMoves = 0;
          for (auto moveset : moves)
          {
            totalMoves += moveset.second;
          }
        }
      }
      ImGui::BeginDisabled(autoMoveGen);
      ImGui::BeginDisabled(data.running);
      if (ImGui::Button("Start"))
      {
        data = game->startPerft(moveGenDepth);
      }
      ImGui::EndDisabled();
      ImGui::BeginDisabled(!data.running);
      ImGui::SameLine();
      if (ImGui::Button("Step"))
      {
        auto res = game->stepPerft(data);
        data.running = !res;
      }
      ImGui::SameLine();
      if (ImGui::Button("Stop"))
      {
        game->stopPerft(data);
      }
      ImGui::EndDisabled();
      ImGui::EndDisabled();

      if (autoMoveGen)
      {
        ImGui::Text("Total moves: %llu", totalMoves);
        for (auto &move : moves)
        {
          ImGui::PushID(move.first.move);
          ImGui::Text("%s: %llu", move.first.getNotation(Chess::Piece::Empty).c_str(), move.second);
          ImGui::SameLine();
          if (ImGui::Button("Make Move"))
          {
            game->makeMove(move.first);
            madeMove = true;
          }
          ImGui::PopID();
        }
        if (ImGui::Button("Copy to Clipboard"))
        {
          std::stringstream stream;
          for (auto move : moves)
          {
            stream << move.first.getNotation(Chess::Piece::Empty) << ", " << move.second << "\n";
          }
          ImGui::SetClipboardText(stream.str().c_str());
        }
      }
    }

    if (open_fen_import)
      ImGui::OpenPopup("Import FEN");
    if (ImGui::BeginPopupModal("Import FEN", &open_fen_import))
    {
      static char fenString[100];
      ImGui::SeparatorText("Paste or type FEN string:");
      ImGui::InputText("FEN", fenString, 100);
      if (ImGui::Button("Cancel"))
      {
        ImGui::CloseCurrentPopup();
        open_fen_import = false;
      }
      ImGui::SameLine();
      if (ImGui::Button("Import"))
      {
        std::string fen{fenString};
        game->importFEN(fen);
        ImGui::CloseCurrentPopup();
        open_fen_import = false;
      }
      ImGui::EndPopup();
    }

    ImGui::End();
  }
};
