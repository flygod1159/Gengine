#include "vPCH.h"
#include <glad/glad.h>
#include "EditorRefactor.h"
#include <engine/Input.h>
#include <engine/gfx/Camera.h>
#include <engine/gfx/api/Framebuffer.h>
#include <engine/gfx/resource/ShaderManager.h>
#include <engine/gfx/Renderer.h>
#include <engine/Scene.h>
#include <game/PlayerActions.h>

#include <engine/ecs/component/Core.h>
#include <engine/ecs/component/Scripting.h>

#include <fstream>
#include <functional>

#include <cereal/types/vector.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/archives/binary.hpp>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <glm/gtc/matrix_transform.hpp>

#include "VoxelManager.h"
#include "prefab.h"

void Editor::SaveRegion()
{
  // prefab-ify the region
  glm::vec3 min(
    glm::min(wpositions[0].x, glm::min(wpositions[1].x, wpositions[2].x)),
    glm::min(wpositions[0].y, glm::min(wpositions[1].y, wpositions[2].y)),
    glm::min(wpositions[0].z, glm::min(wpositions[1].z, wpositions[2].z)));
  glm::vec3 max(
    glm::max(wpositions[0].x, glm::max(wpositions[1].x, wpositions[2].x)),
    glm::max(wpositions[0].y, glm::max(wpositions[1].y, wpositions[2].y)),
    glm::max(wpositions[0].z, glm::max(wpositions[1].z, wpositions[2].z)));
  Voxels::Prefab newPfb;
  for (int x = int(min.x); x <= int(max.x); x++)
  {
    for (int y = int(min.y); y <= int(max.y); y++)
    {
      for (int z = int(min.z); z <= int(max.z); z++)
      {
        // TODO: make bottom-middle of prefab be the origin
        Voxels::Block b = voxels.GetBlock(glm::ivec3(x, y, z));
        if (skipAir && b.GetType() == Voxels::BlockType::bAir)
          continue;
        //b.SetWriteStrength(0x0F);
        newPfb.Add(
          glm::ivec3(x - min.x, y - min.y, z - min.z), b);
      }
    }
  }
  
  // append the prefab to some file
  newPfb.name = sName;
  std::ofstream os("./resources/Prefabs/" + std::string(sName) + ".bin", std::ios::binary);
  cereal::BinaryOutputArchive archive(os);
  archive(newPfb);
}

void Editor::LoadRegion()
{
  auto view = voxels.scene_->GetRegistry().view<Component::Tag>();
  Entity player;
  for (auto entity : view)
  {
    auto tag = view.get<Component::Tag>(entity);
    if (tag.tag == "PlayerSub")
    {
      player = { entity, voxels.scene_ };
      break;
    }
  }
  auto scriptInstance = dynamic_cast<PlayerActions*>(player.GetComponent<Component::NativeScriptComponent>().Instance);
  Voxels::Prefab prefab = Voxels::PrefabManager::LoadPrefabFromFile(std::string(lName));
  scriptInstance->prefab = prefab;
  scriptInstance->prefabName = prefab.name;
}

void Editor::CancelSelection()
{
  selectedPositions = 0;
}

void Editor::SelectBlock()
{
  if (Input::IsKeyPressed(GLFW_KEY_F))
  {
    ASSERT(selectedPositions >= 0 && selectedPositions <= 3);
    wpositions[glm::clamp(selectedPositions, 0, 3)] = hposition;
    if (selectedPositions < 3)
      selectedPositions++;
  }
}

void Editor::DrawSelection(std::span<GFX::RenderView*> renderViews)
{
  {
    ImGui::Begin("Selection Zone");
    int flag = (selectedPositions == 3 ? 0 : ImGuiButtonFlags_Disabled);
    if (ImGui::ButtonEx("save", ImVec2(0, 0), flag))
    {
      // save the prefab in a file or something fam
      SaveRegion();
    }
    ImGui::SameLine();
    ImGui::InputText("##sname", sName, 256);

    if (ImGui::Button("load"))
    {
      LoadRegion();
    }
    ImGui::SameLine();
    ImGui::InputText("##lname", lName, 256);

    ImGui::Checkbox("Skip air?", &skipAir);
    ImGui::Text("Selected positions: %d", selectedPositions);
    ImGui::Text("Hovered   : (%.2f, %.2f, %.2f)", hposition.x, hposition.y, hposition.z);
    ImGui::Text("Position 0: (%.2f, %.2f, %.2f)", wpositions[0].x, wpositions[0].y, wpositions[0].z);
    ImGui::Text("Position 1: (%.2f, %.2f, %.2f)", wpositions[1].x, wpositions[1].y, wpositions[1].z);
    ImGui::Text("Position 2: (%.2f, %.2f, %.2f)", wpositions[2].x, wpositions[2].y, wpositions[2].z);
    ImGui::End();
  }

  {
    // actually draw the bounding box
    glm::vec3 pos(0);
    glm::vec3 scale(0);
    if (selectedPositions == 0)
    {
      scale = glm::vec3(0);
      pos = hposition;
    }
    else if (selectedPositions == 1)
    {
      // system wise
      glm::vec3 min(
        glm::min(wpositions[0].x, hposition.x),
        glm::min(wpositions[0].y, hposition.y),
        glm::min(wpositions[0].z, hposition.z));
      glm::vec3 max(
        glm::max(wpositions[0].x, hposition.x),
        glm::max(wpositions[0].y, hposition.y),
        glm::max(wpositions[0].z, hposition.z));

      pos = (wpositions[0] + hposition) / 2.f;
      scale = glm::abs(max - min);
    }
    else if (selectedPositions == 2)
    {
      // system wise
      glm::vec3 min(
        glm::min(wpositions[0].x, glm::min(wpositions[1].x, hposition.x)),
        glm::min(wpositions[0].y, glm::min(wpositions[1].y, hposition.y)),
        glm::min(wpositions[0].z, glm::min(wpositions[1].z, hposition.z)));
      glm::vec3 max(
        glm::max(wpositions[0].x, glm::max(wpositions[1].x, hposition.x)),
        glm::max(wpositions[0].y, glm::max(wpositions[1].y, hposition.y)),
        glm::max(wpositions[0].z, glm::max(wpositions[1].z, hposition.z)));

      pos = (min + max) / 2.f;
      scale = glm::abs(max - min);
    }
    else// if (selectedPositions == 3)
    {
      glm::vec3 min(
        glm::min(wpositions[0].x, glm::min(wpositions[1].x, wpositions[2].x)),
        glm::min(wpositions[0].y, glm::min(wpositions[1].y, wpositions[2].y)),
        glm::min(wpositions[0].z, glm::min(wpositions[1].z, wpositions[2].z)));
      glm::vec3 max(
        glm::max(wpositions[0].x, glm::max(wpositions[1].x, wpositions[2].x)),
        glm::max(wpositions[0].y, glm::max(wpositions[1].y, wpositions[2].y)),
        glm::max(wpositions[0].z, glm::max(wpositions[1].z, wpositions[2].z)));

      pos = (min + max) / 2.f;
      scale = glm::abs(max - min);
    }

    GLint polygonMode;
    glGetIntegerv(GL_POLYGON_MODE, &polygonMode);
    glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glm::mat4 tPos = glm::translate(glm::mat4(1), pos + .5f);
    glm::mat4 tScale = glm::scale(glm::mat4(1), scale + 1.f);

    auto curr = GFX::ShaderManager::GetShader("flat_color");
    curr->Bind();
    curr->SetMat4("u_model", tPos * tScale);
    curr->SetVec4("u_color", glm::vec4(1.f, .3f, 1.f, 1.f));

    for (auto& renderView : renderViews)
    {
      if (!(renderView->mask & GFX::RenderMaskBit::RenderScreenElements))
        continue;

      // TODO
      //renderView.renderTarget->Bind();

      curr->SetMat4("u_view", renderView->camera->viewInfo.GetViewMatrix());
      curr->SetMat4("u_proj", renderView->camera->projInfo.GetProjMatrix());
      //Renderer::DrawCube(); // TODO
    }

    glPolygonMode(GL_FRONT_AND_BACK, polygonMode);
    glEnable(GL_CULL_FACE);
  }
}

void Editor::Update(Scene* scene)
{
  if (Input::IsKeyPressed(GLFW_KEY_F5))
    open = !open;
  if (open)
  {
    const auto& vi = scene->GetRenderView("main")->camera->viewInfo;

    voxels.Raycast(
      vi.position,
      vi.GetForwardDir(),
      pickLength,
      [&](glm::vec3 pos, Voxels::Block block, [[maybe_unused]] glm::vec3 side)->bool
    {
      if (block.GetType() == Voxels::BlockType::bAir)
        return false;
      if (selectedPositions == 0)
        hposition = pos;
      else if (selectedPositions == 1)
      {
        // find axis that has smallest difference, and lock that one
        glm::vec3 diff = pos - wpositions[0];
        diff = glm::abs(diff);
        float smol = std::min(diff.x, std::min(diff.y, diff.z));
        if (smol == diff.x)
          hposition = glm::vec3(wpositions[0].x, pos.y, pos.z);
        else if (smol == diff.y)
          hposition = glm::vec3(pos.x, wpositions[0].y, pos.z);
        else if (smol == diff.z)
          hposition = glm::vec3(pos.x, pos.y, wpositions[0].z);
      }
      else if (selectedPositions == 2)
      {
        // only move the axis that is shared between first two positions
        glm::vec3 diff = wpositions[1] - wpositions[0];
        hposition = wpositions[0];
        if (!diff.x)
          hposition.x = pos.x;
        if (!diff.y)
          hposition.y = pos.y;
        if (!diff.z)
          hposition.z = pos.z;
      }
      SelectBlock();
      return true;
    });

    auto renderViews = scene->GetRenderViews();
    DrawSelection(renderViews);
  }
  else
  {
    CancelSelection();
  }
}