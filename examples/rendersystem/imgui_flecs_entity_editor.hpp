#pragma once

#include <map>
#include <set>
#include <functional>
#include <string>
#include <flecs.h>
#include <imgui.h>

#ifndef MM_IEEE_ASSERT
#define MM_IEEE_ASSERT(x) assert(x)
#endif

#define MM_IEEE_IMGUI_PAYLOAD_TYPE_ENTITY "MM_IEEE_ENTITY"

#ifndef MM_IEEE_ENTITY_WIDGET
#define MM_IEEE_ENTITY_WIDGET ::MM::EntityWidget
#endif

namespace MM {

	inline void EntityWidget(ecs_entity_t& e, flecs::world& world, bool dropTarget = false)
	{
		ImGui::PushID(static_cast<int>(e & 0x7FFFFFFF));

		if (e != 0 && world.is_alive(e)) {
			ImGui::Text("ID: %llu", (unsigned long long)e);
		}
		else {
			ImGui::Text("Invalid Entity");
		}

		if (e != 0 && world.is_alive(e)) {
			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
				ImGui::SetDragDropPayload(MM_IEEE_IMGUI_PAYLOAD_TYPE_ENTITY, &e, sizeof(e));
				ImGui::Text("ID: %llu", (unsigned long long)e);
				ImGui::EndDragDropSource();
			}
		}

		if (dropTarget && ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(MM_IEEE_IMGUI_PAYLOAD_TYPE_ENTITY)) {
				e = *(ecs_entity_t*)payload->Data;
			}
			ImGui::EndDragDropTarget();
		}

		ImGui::PopID();
	}

	template <class Component>
	void ComponentEditorWidget([[maybe_unused]] flecs::world& world, [[maybe_unused]] ecs_entity_t entity) {}

	template <class Component>
	void ComponentAddAction(flecs::world& world, ecs_entity_t entity)
	{
		world.ensure<Component>(entity);
	}

	template <class Component>
	void ComponentRemoveAction(flecs::world& world, ecs_entity_t entity)
	{
		world.remove<Component>(entity);
	}

	class FlecsEntityEditor {
	public:
		using ComponentTypeID = ecs_entity_t;
		using Callback = std::function<void(flecs::world&, ecs_entity_t)>;

		struct ComponentInfo {
			std::string name;
			Callback widget, create, destroy;
		};

		bool show_window = true;

	private:
		std::map<ComponentTypeID, ComponentInfo> component_infos;

		template <class Component>
		static ComponentTypeID typeId(flecs::world& world)
		{
			return world.id<Component>();
		}

		bool entityHasComponent(flecs::world& world, ecs_entity_t entity, ComponentTypeID type_id)
		{
			return ecs_has_id(world.c_ptr(), entity, type_id);
		}

	public:
		template <class Component>
		ComponentInfo& registerComponent(flecs::world& world, const std::string& name)
		{
			auto id = world.id<Component>();
			component_infos[id] = ComponentInfo{
				name,
				ComponentEditorWidget<Component>,
				ComponentAddAction<Component>,
				ComponentRemoveAction<Component>,
			};
			return component_infos[id];
		}

		template <class Component>
		ComponentInfo& registerComponent(flecs::world& world, const std::string& name, Callback widget)
		{
			auto id = world.id<Component>();
			component_infos[id] = ComponentInfo{
				name,
				widget,
				ComponentAddAction<Component>,
				ComponentRemoveAction<Component>,
			};
			return component_infos[id];
		}

		void renderEditor(flecs::world& world, ecs_entity_t& e)
		{
			ImGui::TextUnformatted("Editing:");
			ImGui::SameLine();
			MM_IEEE_ENTITY_WIDGET(e, world, true);

			if (ImGui::Button("New")) {
				e = world.entity().id();
			}
			if (e != 0 && world.is_alive(e)) {
				ImGui::SameLine();
				if (ImGui::Button("Clone")) {
					auto old_e = e;
					e = world.entity().id();
					ecs_clone(world.c_ptr(), e, old_e, true);
				}
				ImGui::SameLine();
				ImGui::Dummy({ 10, 0 });
				ImGui::SameLine();
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.65f, 0.15f, 0.15f, 1.f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.3f, 0.3f, 1.f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.f, 0.2f, 0.2f, 1.f));
				if (ImGui::Button("Destroy")) {
					ecs_delete(world, e);
					e = 0;
				}
				ImGui::PopStyleColor(3);
			}

			ImGui::Separator();

			if (e != 0 && world.is_alive(e)) {
				ImGui::PushID(static_cast<int>(e & 0x7FFFFFFF));
				std::map<ComponentTypeID, ComponentInfo> has_not;
				for (auto& [component_type_id, ci] : component_infos) {
					if (component_type_id == 0) continue;
					if (entityHasComponent(world, e, component_type_id)) {
						ImGui::PushID(static_cast<int>(component_type_id & 0x7FFFFFFF));
						if (ImGui::Button("-")) {
							ci.destroy(world, e);
							ImGui::PopID();
							continue;
						}
						else {
							ImGui::SameLine();
						}
						if (ImGui::CollapsingHeader(ci.name.c_str())) {
							ImGui::Indent(30.f);
							ImGui::PushID("Widget");
							ci.widget(world, e);
							ImGui::PopID();
							ImGui::Unindent(30.f);
						}
						ImGui::PopID();
					}
					else {
						has_not[component_type_id] = ci;
					}
				}
				if (!has_not.empty()) {
					if (ImGui::Button("+ Add Component")) {
						ImGui::OpenPopup("Add Component");
					}
					if (ImGui::BeginPopup("Add Component")) {
						ImGui::TextUnformatted("Available:");
						ImGui::Separator();
						for (auto& [component_type_id, ci] : has_not) {
							if (component_type_id == 0) continue;
							ImGui::PushID(static_cast<int>(component_type_id & 0x7FFFFFFF));
							if (ImGui::Selectable(ci.name.c_str())) {
								ci.create(world, e);
							}
							ImGui::PopID();
						}
						ImGui::EndPopup();
					}
				}
				ImGui::PopID();
			}
		}

		void renderEntityList(flecs::world& world, std::set<ComponentTypeID>& comp_list)
		{
			ImGui::Text("Components Filter:");
			ImGui::SameLine();
			if (ImGui::SmallButton("clear")) {
				comp_list.clear();
			}
			ImGui::Indent();
			for (const auto& [component_type_id, ci] : component_infos) {
				if (component_type_id == 0) continue;
				bool is_in_list = comp_list.count(component_type_id);
				bool active = is_in_list;
				ImGui::Checkbox(ci.name.c_str(), &active);
				if (is_in_list && !active)
					comp_list.erase(component_type_id);
				else if (!is_in_list && active)
					comp_list.emplace(component_type_id);
			}
			ImGui::Unindent();
			ImGui::Separator();

			if (comp_list.empty()) {
				ImGui::Text("Entities (no filter):");
				if (ImGui::BeginChild("entity list")) {
					world.each([&](flecs::entity e) {
						ecs_entity_t id = e.id();
						MM_IEEE_ENTITY_WIDGET(id, world, false);
					});
				}
				ImGui::EndChild();
			}
			else {
				ImGui::Text("Entities Matching:");
				std::set<ecs_entity_t> entity_ids;
				for (ComponentTypeID tid : comp_list) {
					world.filter_builder().with(tid).build().each([&](flecs::entity e) {
						entity_ids.insert(e.id());
					});
				}
				if (ImGui::BeginChild("entity list")) {
					for (ecs_entity_t id : entity_ids) {
						MM_IEEE_ENTITY_WIDGET(id, world, false);
					}
				}
				ImGui::EndChild();
			}
		}

		void renderSimpleCombo(flecs::world& world, ecs_entity_t& e)
		{
			if (show_window) {
				ImGui::SetNextWindowSize(ImVec2(550, 400), ImGuiCond_FirstUseEver);
				if (ImGui::Begin("Entity Editor", &show_window)) {
					if (ImGui::BeginChild("list", { 200, 0 }, true)) {
						static std::set<ComponentTypeID> comp_list;
						renderEntityList(world, comp_list);
					}
					ImGui::EndChild();
					ImGui::SameLine();
					if (ImGui::BeginChild("editor")) {
						renderEditor(world, e);
					}
					ImGui::EndChild();
				}
				ImGui::End();
			}
		}
	};
}
