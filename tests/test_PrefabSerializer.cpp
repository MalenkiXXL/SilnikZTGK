#include <gtest/gtest.h>
#include <fstream>
#include <nlohmann/json.hpp>

// Pamiętaj o poprawnych ścieżkach do Twoich nagłówków!
#include "../SilnikZTGK/CookingStation/Scene/Scene.h"
#include "../SilnikZTGK/CookingStation/Scene/PrefabSerializer.h"
#include "../SilnikZTGK/CookingStation/Core/VFS/VFS.h" // Jeśli VFS wymaga inicjalizacji

/**
 * @file PrefabSerializerTests.cpp
 * @brief Zestaw testów jednostkowych weryfikujących poprawność ładowania prefabów z JSON.
 * Główny nacisk na sprawdzanie odbudowy hierarchii rodzic-dziecko w ECS.
 */

class PrefabSerializerTest : public ::testing::Test {
protected:
    std::shared_ptr<Scene> testScene;
    std::string testFilePath = "test_prefab_hierarchy.json";

    void SetUp() override {
        // Inicjalizacja pustej sceny
        testScene = std::make_shared<Scene>();

        // UWAGA: Jeśli Twój VFS wymaga inicjalizacji przed użyciem VFS::ReadFile,
        // zrób to tutaj (np. VFS::Mount("assets", "./Assets")).
    }

    void TearDown() override {
        // Sprzątanie: usuwamy tymczasowy plik po teście
        std::remove(testFilePath.c_str());
    }

    // Helper do generowania tymczasowego pliku JSON na dysku
    void CreateTestJsonFile(const nlohmann::json &jsonData) {
        std::ofstream file(testFilePath);
        ASSERT_TRUE(file.is_open()) << "Nie udało się utworzyć pliku testowego!";
        file << jsonData.dump(4);
        file.close();
    }
};

// ---------------------------------------------------------
// TEST 1: Weryfikacja pojedynczego rodzica i jednego dziecka
// ---------------------------------------------------------
TEST_F(PrefabSerializerTest, DeserializesSingleParentChildRelationship
) {
// Arrange: Tworzymy JSON z jednym rodzicem i jednym dzieckiem
    nlohmann::json prefabJson = {
            {
                    {"local_id", 1},
                    {"name",      "ParentCar"}
            },
            {
                    {"local_id", 2},
                    {"parent_id", 1},
                    {"name", "TrunkDoor"}
            }
    };
    CreateTestJsonFile(prefabJson);

// Act: Deserializujemy nasz testowy plik
    glm::vec3 spawnPos(0.0f, 0.0f, 0.0f);

// UWAGA: Ścieżka zależy od tego, jak VFS ją rozwiązuje.
// Możesz użyć wprost ścieżki do utworzonego testFilePath.
    std::vector<Entity> createdEntities = PrefabSerializer::Deserialize(testScene.get(), testFilePath, spawnPos);

// Assert: Sprawdzamy podstawowe założenia
    ASSERT_EQ(createdEntities
                      .

                              size(),

              2) << "Serializer powinien utworzyć dokładnie 2 encje.";

    Entity parentEntity = createdEntities[0];
    Entity childEntity = createdEntities[1];

    auto &world = testScene->GetWorld();
    auto *parentRel = world.GetComponent<RelationshipComponent>(parentEntity);
    auto *childRel = world.GetComponent<RelationshipComponent>(childEntity);

// Weryfikacja czy w ogóle dodano komponenty relacji
    ASSERT_NE(parentRel,
              nullptr);
    ASSERT_NE(childRel,
              nullptr);

// Weryfikacja spójności hierarchii
    EXPECT_EQ(parentRel
                      ->ChildrenCount, 1);
    EXPECT_EQ(parentRel
                      ->FirstChild, childEntity.id);

    EXPECT_EQ(childRel
                      ->Parent, parentEntity.id);
    EXPECT_EQ(childRel
                      ->NextSibling, NULL_ENTITY);
    EXPECT_EQ(childRel
                      ->PreviousSibling, NULL_ENTITY);
}

// ---------------------------------------------------------
// TEST 2: Weryfikacja jednego rodzica i wielu dzieci (rodzeństwo)
// ---------------------------------------------------------
TEST_F(PrefabSerializerTest, DeserializesMultipleSiblingsCorrectly
) {
// Arrange: Tworzymy JSON z jednym rodzicem i dwójką dzieci
    nlohmann::json prefabJson = {
            {
                    {"local_id", 10},
                    {"name",      "DeliveryCar"}
            },
            {
                    {"local_id", 11},
                    {"parent_id", 10},
                    {"name", "Door_Left"}
            },
            {
                    {"local_id", 12},
                    {"parent_id", 10},
                    {"name", "Door_Right"}
            }
    };
    CreateTestJsonFile(prefabJson);

// Act: Deserializacja
    std::vector<Entity> createdEntities = PrefabSerializer::Deserialize(testScene.get(), testFilePath, glm::vec3(0.0f));
    ASSERT_EQ(createdEntities
                      .

                              size(),

              3);

    Entity parentEntity = createdEntities[0];
    Entity child1 = createdEntities[1];
    Entity child2 = createdEntities[2];

    auto &world = testScene->GetWorld();
    auto *parentRel = world.GetComponent<RelationshipComponent>(parentEntity);
    auto *c1Rel = world.GetComponent<RelationshipComponent>(child1);
    auto *c2Rel = world.GetComponent<RelationshipComponent>(child2);

// Assert: Sprawdzanie Parenta
    ASSERT_NE(parentRel,
              nullptr);
    EXPECT_EQ(parentRel
                      ->ChildrenCount, 2);
    EXPECT_EQ(parentRel
                      ->FirstChild, child1.id);

// Assert: Sprawdzanie pierwszego dziecka (Left Door)
    ASSERT_NE(c1Rel,
              nullptr);
    EXPECT_EQ(c1Rel
                      ->Parent, parentEntity.id);
    EXPECT_EQ(c1Rel
                      ->NextSibling, child2.id) << "Dziecko 1 powinno wskazywać na Dziecko 2 jako następne rodzeństwo";
    EXPECT_EQ(c1Rel
                      ->PreviousSibling, NULL_ENTITY);

// Assert: Sprawdzanie drugiego dziecka (Right Door)
    ASSERT_NE(c2Rel,
              nullptr);
    EXPECT_EQ(c2Rel
                      ->Parent, parentEntity.id);
    EXPECT_EQ(c2Rel
                      ->NextSibling, NULL_ENTITY) << "Dziecko 2 jest ostatnie, brak następnego rodzeństwa";
    EXPECT_EQ(c2Rel
                      ->PreviousSibling, child1.id)
                        << "Dziecko 2 powinno wskazywać na Dziecko 1 jako poprzednie rodzeństwo";
}

// ---------------------------------------------------------
// TEST 3: Weryfikacja hierarchii kaskadowej (Dziadek -> Ojciec -> Syn)
// ---------------------------------------------------------
TEST_F(PrefabSerializerTest, DeserializesDeepHierarchy
) {
// Arrange: Drzewo wielopoziomowe
    nlohmann::json prefabJson = {
            {{"local_id", 1}, {"name",      "Grandparent"}},
            {{"local_id", 2}, {"parent_id", 1}, {"name", "Parent"}},
            {{"local_id", 3}, {"parent_id", 2}, {"name", "Child"}}
    };
    CreateTestJsonFile(prefabJson);

// Act
    std::vector<Entity> createdEntities = PrefabSerializer::Deserialize(testScene.get(), testFilePath, glm::vec3(0.0f));
    ASSERT_EQ(createdEntities
                      .

                              size(),

              3);

    auto &world = testScene->GetWorld();
    auto *grandRel = world.GetComponent<RelationshipComponent>(createdEntities[0]);
    auto *parentRel = world.GetComponent<RelationshipComponent>(createdEntities[1]);
    auto *childRel = world.GetComponent<RelationshipComponent>(createdEntities[2]);

// Assert
// Dziadek
    EXPECT_EQ(grandRel
                      ->ChildrenCount, 1);
    EXPECT_EQ(grandRel
                      ->FirstChild, createdEntities[1].id);

// Rodzic (Ojciec)
    EXPECT_EQ(parentRel
                      ->Parent, createdEntities[0].id);
    EXPECT_EQ(parentRel
                      ->ChildrenCount, 1);
    EXPECT_EQ(parentRel
                      ->FirstChild, createdEntities[2].id);

// Dziecko (Syn)
    EXPECT_EQ(childRel
                      ->Parent, createdEntities[1].id);
    EXPECT_EQ(childRel
                      ->ChildrenCount, 0);
}