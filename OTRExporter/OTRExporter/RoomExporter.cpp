#define NO_GDI
#define WIN32_LEAN_AND_MEAN
#include "RoomExporter.h"
#include "Utils/BinaryWriter.h"
#include "Utils/MemoryStream.h"
#include <Utils/DiskFile.h>
#include <ZRoom/Commands/SetMesh.h>
#include <ZRoom/Commands/SetWind.h>
#include <ZRoom/Commands/SetTimeSettings.h>
#include <ZRoom/Commands/SetSkyboxModifier.h>
#include <ZRoom/Commands/SetSoundSettings.h>
#include <ZRoom/Commands/SetCameraSettings.h>
#include <ZRoom/Commands/SetRoomBehavior.h>
#include <ZRoom/Commands/SetCsCamera.h>
#include <ZRoom/Commands/SetRoomList.h>
#include <ZRoom/Commands/SetCollisionHeader.h>
#include <ZRoom/Commands/SetEntranceList.h>
#include <ZRoom/Commands/SetSpecialObjects.h>
#include <ZRoom/Commands/SetStartPositionList.h>
#include <ZRoom/Commands/SetSkyboxSettings.h>
#include <ZRoom/Commands/SetLightingSettings.h>
#include <ZRoom/Commands/SetLightList.h>
#include <ZRoom/Commands/SetEchoSettings.h>
#include <ZRoom/Commands/SetObjectList.h>
#include <ZRoom/Commands/SetAlternateHeaders.h>
#include "CollisionExporter.h"
#include "DisplayListExporter.h"
#include <libultraship/bridge.h>
#include <Globals.h>
#include <ZRoom/Commands/SetExitList.h>
#include <ZRoom/Commands/SetPathways.h>
#include "TextureExporter.h"
#include "Main.h"
#include <ZRoom/Commands/SetCutscenes.h>
#include "CutsceneExporter.h"
#include <ZRoom/Commands/SetTransitionActorList.h>
#include <ZRoom/Commands/SetCutsceneEntryList.h>
#include <ZRoom/Commands/SetAnimatedMaterialList.h>
#include <ZRoom/Commands/SetMinimapList.h>
#include <ZRoom/Commands/SetMinimapChests.h>
#include "TextureAnimationExporter.h"
#include "PathExporter.h"
#undef FindResource


void OTRExporter_Room::Save(ZResource* res, const fs::path& outPath, BinaryWriter* writer)
{
    ZRoom* room = (ZRoom*)res;

    WriteHeader(res, outPath, writer, static_cast<uint32_t>(SOH::ResourceType::SOH_Room));

    writer->Write((uint32_t)room->commands.size());

    for (size_t i = 0; i < room->commands.size(); i++)
    {
        ZRoomCommand* cmd = room->commands[i];

        writer->Write((uint32_t)cmd->cmdID);

        switch (cmd->cmdID)
        {
        case RoomCommand::SetTransitionActorList:
        {
            SetTransitionActorList* cmdTrans = (SetTransitionActorList*)cmd;

            writer->Write((uint32_t)cmdTrans->transitionActors.size());

            for (const TransitionActorEntry& entry : cmdTrans->transitionActors)
            {
                writer->Write(entry.frontObjectRoom);
                writer->Write(entry.frontTransitionReaction);
                writer->Write(entry.backObjectRoom);
                writer->Write(entry.backTransitionReaction);
                writer->Write(entry.actorNum);
                writer->Write(entry.posX);
                writer->Write(entry.posY);
                writer->Write(entry.posZ);
                writer->Write(entry.rotY);
                writer->Write(entry.initVar);
            }
        }
            break;
        case RoomCommand::SetActorList:
        {
            SetActorList* cmdSetActorList = (SetActorList*)cmd;

            // There are instance of the amount of actors in the file differing from the size listed in the command.
            // This can cause issues if we export actors with garbage data, so let's trust the command size
            writer->Write((uint32_t)cmdSetActorList->numActors);

            for (const auto& entry : cmdSetActorList->actorList->actors)
            {
                writer->Write(entry.actorNum);
                writer->Write(entry.posX);
                writer->Write(entry.posY);
                writer->Write(entry.posZ);
                writer->Write(entry.rotX);
                writer->Write(entry.rotY);
                writer->Write(entry.rotZ);
                writer->Write(entry.params);
            }
        }
            break;
        case RoomCommand::SetWind:
        {
            SetWind* cmdSetWind = (SetWind*)cmd;

            writer->Write(cmdSetWind->windWest); // 0x04
            writer->Write(cmdSetWind->windVertical); // 0x05
            writer->Write(cmdSetWind->windSouth); // 0x06
            writer->Write(cmdSetWind->clothFlappingStrength); // 0x07
        }
            break;
        case RoomCommand::SetTimeSettings:
        {
            SetTimeSettings* cmdTime = (SetTimeSettings*)cmd;

            writer->Write(cmdTime->hour); // 0x04
            writer->Write(cmdTime->min); // 0x05
            writer->Write(cmdTime->unk); // 0x06
        }
        break;
        case RoomCommand::SetSkyboxModifier:
        {
            SetSkyboxModifier* cmdSkybox = (SetSkyboxModifier*)cmd;

            writer->Write(cmdSkybox->disableSky); // 0x04
            writer->Write(cmdSkybox->disableSunMoon); // 0x05
        }
        break;
        case RoomCommand::SetEchoSettings:
        {
            SetEchoSettings* cmdEcho = (SetEchoSettings*)cmd;

            writer->Write((uint8_t)cmdEcho->echo); // 0x07
        }
        break;
        case RoomCommand::SetSoundSettings:
        {
            SetSoundSettings* cmdSound = (SetSoundSettings*)cmd;

            writer->Write((uint8_t)cmdSound->reverb); // 0x01

            writer->Write(cmdSound->nightTimeSFX); // 0x06
            writer->Write(cmdSound->musicSequence); // 0x07
        }
        break;
        case RoomCommand::SetSkyboxSettings:
        {
            SetSkyboxSettings* cmdSkybox = (SetSkyboxSettings*)cmd;

            writer->Write((uint8_t)cmdSkybox->unk1); // 0x01
            writer->Write((uint8_t)cmdSkybox->skyboxNumber); // 0x04
            writer->Write((uint8_t)cmdSkybox->cloudsType); // 0x05
            writer->Write((uint8_t)cmdSkybox->isIndoors); // 0x06
        }
        break;
        case RoomCommand::SetRoomBehavior:
        {
            SetRoomBehavior* cmdRoom = (SetRoomBehavior*)cmd;
            if (Globals::Instance->game == ZGame::MM_RETAIL) {
                writer->Write(cmdRoom->gameplayFlags);
                writer->Write(cmdRoom->currRoomUnk2);
                writer->Write(cmdRoom->currRoomUnk5);
                writer->Write(cmdRoom->msgCtxUnk);
                writer->Write(cmdRoom->enablePosLights);
                writer->Write(cmdRoom->kankyoContextUnkE2);
            } else {
                writer->Write((uint8_t)cmdRoom->gameplayFlags); // 0x01
                writer->Write(cmdRoom->gameplayFlags2); // 0x04
            }
        }
        break;
        case RoomCommand::SetCsCamera:
        {
            SetCsCamera* cmdCsCam = (SetCsCamera*)cmd;

            writer->Write((uint32_t)cmdCsCam->cameras.size());
            segptr_t arrBase = cmdCsCam->cameras[0].segmentOffset;
            for (const auto& c : cmdCsCam->cameras) {
                writer->Write(c.type);
                writer->Write(c.numPoints);
                for (int16_t i = 0; i < c.numPoints; i++) {
                    writer->Write(cmdCsCam->points[((c.segmentOffset - arrBase) / 6) + i].scalars[0].scalarData.s16);
                    writer->Write(cmdCsCam->points[((c.segmentOffset - arrBase) / 6) + i].scalars[1].scalarData.s16);
                    writer->Write(cmdCsCam->points[((c.segmentOffset - arrBase) / 6) + i].scalars[2].scalarData.s16);
                }
            }
        }
        break;
        case RoomCommand::SetMesh:
        {
            SetMesh* cmdMesh = (SetMesh*)cmd;

            writer->Write((uint8_t)cmdMesh->data); // 0x01
            writer->Write(cmdMesh->meshHeaderType);

            if (cmdMesh->meshHeaderType == 0 || cmdMesh->meshHeaderType == 2)
            {
                RoomShapeCullable* poly = (RoomShapeCullable*)cmdMesh->polyType.get();

                writer->Write(poly->num);

                for (int i = 0; i < poly->num; i++)
                    WritePolyDList(writer, room, &poly->polyDLists[i]);
            }
            else if (cmdMesh->meshHeaderType == 1)
            {
                PolygonType1* poly = (PolygonType1*)cmdMesh->polyType.get();

                writer->Write(poly->format);

                auto test = (RoomShapeDListsEntry*)&poly->polyDLists[0];
                Declaration* dListDeclOpa = poly->parent->GetDeclaration(GETSEGOFFSET(test->opa));
                Declaration* dListDeclXlu = poly->parent->GetDeclaration(GETSEGOFFSET(test->xlu));

                if (test->opa != 0)
                    writer->Write(StringHelper::Sprintf("%s/%s", OTRExporter_DisplayList::GetParentFolderName(res).c_str(), dListDeclOpa->declName.c_str()));
                else
                    writer->Write("");

                if (test->xlu != 0)
                    writer->Write(StringHelper::Sprintf("%s/%s", OTRExporter_DisplayList::GetParentFolderName(res).c_str(), dListDeclXlu->declName.c_str()));
                else
                    writer->Write("");

                if (poly->format == 2)
                {
                    writer->Write((uint32_t)poly->count);

                    for (int i = 0; i < poly->count; i++)
                    {
                        writer->Write(poly->multiList[i].unk_00);
                        writer->Write(poly->multiList[i].id);

                        Declaration* bgDecl = poly->parent->GetDeclarationRanged(GETSEGOFFSET(poly->multiList[i].source));

                        writer->Write(OTRExporter_DisplayList::GetPathToRes(poly->multiList[i].sourceBackground, bgDecl->declName));

                        writer->Write(poly->multiList[i].unk_0C);
                        writer->Write(poly->multiList[i].tlut);
                        writer->Write(poly->multiList[i].width);
                        writer->Write(poly->multiList[i].height);
                        writer->Write(poly->multiList[i].fmt);
                        writer->Write(poly->multiList[i].siz);
                        writer->Write(poly->multiList[i].mode0);
                        writer->Write(poly->multiList[i].tlutCount);
                    }
                }
                else
                {
                    writer->Write((uint32_t)1);

                    writer->Write(poly->single.unk_00);
                    writer->Write(poly->single.id);

                    Declaration* bgDecl = poly->parent->GetDeclarationRanged(GETSEGOFFSET(poly->single.source));

                    writer->Write(OTRExporter_DisplayList::GetPathToRes(poly->single.sourceBackground, bgDecl->declName));

                    writer->Write(poly->single.unk_0C);
                    writer->Write(poly->single.tlut);
                    writer->Write(poly->single.width);
                    writer->Write(poly->single.height);
                    writer->Write(poly->single.fmt);
                    writer->Write(poly->single.siz);
                    writer->Write(poly->single.mode0);
                    writer->Write(poly->single.tlutCount);
                }

                if (poly->dlist != 0)
                    WritePolyDList(writer, room, &poly->polyDLists[0]);
            }
        }
        break;
        case RoomCommand::SetCameraSettings:
        //case RoomCommand::SetWorldMapVisited: MM command but you can't have 0x19 twice in a switch
        {
            if (Globals::Instance->game != ZGame::MM_RETAIL) {
                SetCameraSettings* cmdCam = (SetCameraSettings*)cmd;

                writer->Write((uint8_t)cmdCam->cameraMovement); // 0x01
                writer->Write(cmdCam->mapHighlight); // 0x04
            }
        }
        break;
        case RoomCommand::SetLightList:
        {
            SetLightList* cmdLight = (SetLightList*)cmd;

            writer->Write((uint32_t)cmdLight->lights.size());

            for (size_t i = 0; i < cmdLight->lights.size(); i++)
            {
                writer->Write(cmdLight->lights[i].type);
                writer->Write(cmdLight->lights[i].x);
                writer->Write(cmdLight->lights[i].y);
                writer->Write(cmdLight->lights[i].z);
                writer->Write(cmdLight->lights[i].r);
                writer->Write(cmdLight->lights[i].g);
                writer->Write(cmdLight->lights[i].b);
                writer->Write(cmdLight->lights[i].drawGlow);
                writer->Write(cmdLight->lights[i].radius);
            }
        }
            break;
        case RoomCommand::SetLightingSettings:
        {
            SetLightingSettings* cmdLight = (SetLightingSettings*)cmd;

            writer->Write((uint32_t)cmdLight->settings.size()); // 0x01

            for (const LightingSettings& setting : cmdLight->settings)
            {
                writer->Write(setting.ambientClrR);
                writer->Write(setting.ambientClrG);
                writer->Write(setting.ambientClrB);

                writer->Write(setting.diffuseClrA_R);
                writer->Write(setting.diffuseClrA_G);
                writer->Write(setting.diffuseClrA_B);

                writer->Write(setting.diffuseDirA_X);
                writer->Write(setting.diffuseDirA_Y);
                writer->Write(setting.diffuseDirA_Z);

                writer->Write(setting.diffuseClrB_R);
                writer->Write(setting.diffuseClrB_G);
                writer->Write(setting.diffuseClrB_B);

                writer->Write(setting.diffuseDirB_X);
                writer->Write(setting.diffuseDirB_Y);
                writer->Write(setting.diffuseDirB_Z);

                writer->Write(setting.fogClrR);
                writer->Write(setting.fogClrG);
                writer->Write(setting.fogClrB);

                writer->Write(setting.unk);
                writer->Write(setting.drawDistance);
            }
        }
        break;
        case RoomCommand::SetRoomList:
        {
            SetRoomList* cmdRoom = (SetRoomList*)cmd;

            writer->Write((uint32_t)cmdRoom->romfile->numRooms); // 0x01

            for (size_t i = 0;i < cmdRoom->romfile->numRooms; i++)
            {
                //std::string roomName = StringHelper::Sprintf("%s/%s_room_%i", (StringHelper::Split(room->GetName(), "_")[0] + "_scene").c_str(), StringHelper::Split(room->GetName(), "_scene")[0].c_str(), i);
                std::string roomName;
                if (Globals::Instance->game != ZGame::MM_RETAIL)
                    roomName = OTRExporter_DisplayList::GetPathToRes(room, StringHelper::Sprintf("%s_room_%i", StringHelper::Split(room->GetName(), "_scene")[0].c_str(), i));
                else
                {
                    // Alt headers use the rooms from their parent room. For example we want SPOT00/SPOT00_room_00, not SPOT00/SPOT00Set_00A050_room_00
                    if (room->zroomType == ZResourceType::AltHeader)
                        roomName = OTRExporter_DisplayList::GetPathToRes(room, StringHelper::Sprintf("%s_room_%02d", cmd->parent->GetName().c_str(), i));
                    else
                        roomName = OTRExporter_DisplayList::GetPathToRes(room, StringHelper::Sprintf("%s_room_%02d", StringHelper::Split(room->GetName(), "_scene")[0].c_str(), i));
                }

                writer->Write(roomName);
                writer->Write(cmdRoom->romfile->rooms[i].virtualAddressStart);
                writer->Write(cmdRoom->romfile->rooms[i].virtualAddressEnd);
            }
        }
        break;
        case RoomCommand::SetCollisionHeader:
        {
            SetCollisionHeader* cmdCollHeader = (SetCollisionHeader*)cmd;

            Declaration* colHeaderDecl = room->parent->GetDeclaration(cmdCollHeader->segmentOffset);
            std::string path = OTRExporter_DisplayList::GetPathToRes(room, colHeaderDecl->declName);
            writer->Write(path);
        }
        break;
        case RoomCommand::SetEntranceList:
        {
            SetEntranceList* cmdEntrance = (SetEntranceList*)cmd;

            writer->Write((uint32_t)cmdEntrance->entrances.size());

            for (Spawn entry : cmdEntrance->entrances)
            {
                writer->Write((uint8_t)entry.startPositionIndex);
                writer->Write((uint8_t)entry.roomToLoad);
            }
        }
        break;
        case RoomCommand::SetSpecialObjects:
        {
            SetSpecialObjects* cmdSpecObj = (SetSpecialObjects*)cmd;

            writer->Write((uint8_t)cmdSpecObj->elfMessage); // 0x01
            writer->Write((uint16_t)cmdSpecObj->globalObject); // 0x06
        }
        break;
        case RoomCommand::SetStartPositionList:
        {
            SetStartPositionList* cmdStartPos = (SetStartPositionList*)cmd;

            uint32_t baseStreamEnd = writer->GetStream().get()->GetLength();

            writer->Write((uint32_t)cmdStartPos->actors.size()); // 0x01

            for (const ActorSpawnEntry& entry : cmdStartPos->actors)
            {
                writer->Write(entry.actorNum);
                writer->Write(entry.posX);
                writer->Write(entry.posY);
                writer->Write(entry.posZ);
                writer->Write(entry.rotX);
                writer->Write(entry.rotY);
                writer->Write(entry.rotZ);
                writer->Write(entry.params);
            }
        }
        break;
        case RoomCommand::SetAlternateHeaders:
        {
            SetAlternateHeaders* cmdHeaders = (SetAlternateHeaders*)cmd;

            writer->Write((uint32_t)cmdHeaders->headers.size());

            for (size_t i = 0; i < cmdHeaders->headers.size(); i++)
            {
                uint32_t seg = cmdHeaders->headers[i] & 0xFFFFFFFF;
                std::string headerName = "";
                bool foundDecl = Globals::Instance->GetSegmentedPtrName(seg, room->parent, "", headerName, res->parent->workerID);
                if (headerName == "NULL")
                    writer->Write("");
                else
                {
                    std::string name = OTRExporter_DisplayList::GetPathToRes(room, headerName);
                    writer->Write(name);
                }
            }
        }
            break;
        case RoomCommand::SetExitList:
        {
            SetExitList* cmdExit = (SetExitList*)cmd;

            writer->Write((uint32_t)cmdExit->exits.size());

            for (size_t i = 0; i < cmdExit->exits.size(); i++)
                writer->Write(cmdExit->exits[i]);
        }
            break;
        case RoomCommand::SetObjectList:
        {
            SetObjectList* cmdSetObjectList = (SetObjectList*)cmd;

            writer->Write((uint32_t)cmdSetObjectList->objects.size());

            for (size_t i = 0; i < cmdSetObjectList->objects.size(); i++)
                writer->Write(cmdSetObjectList->objects[i]);
        }
            break;
        case RoomCommand::SetCutscenes:
        {
            SetCutscenes* cmdSetCutscenes = (SetCutscenes*)cmd;
            
            std::string listName;
            if (Globals::Instance->game == ZGame::MM_RETAIL) {
                writer->Seek(-4, SeekOffsetType::Current);
                writer->Write((uint32_t)RoomCommand::SetCutscenesMM);
                writer->Write(cmdSetCutscenes->numCutscenes);
                for (size_t i = 0; i < cmdSetCutscenes->cutsceneEntries.size(); i++) {
                    Globals::Instance->GetSegmentedPtrName(cmdSetCutscenes->cutsceneEntries[i].segmentPtr, room->parent, "CutsceneData", listName, res->parent->workerID);
                    std::string fName = OTRExporter_DisplayList::GetPathToRes(room, listName);
                    writer->Write(fName);
                    writer->Write(cmdSetCutscenes->cutsceneEntries[i].exit);
                    writer->Write(cmdSetCutscenes->cutsceneEntries[i].entrance);
                    writer->Write(cmdSetCutscenes->cutsceneEntries[i].flag);
                    
                    MemoryStream* csStream = new MemoryStream();
                    BinaryWriter csWriter = BinaryWriter(csStream);
                    OTRExporter_Cutscene cs;
                    ZResource* newCs = res->parent->FindResource(cmdSetCutscenes->cutsceneEntries[i].segmentPtr & 0x00FFFFFF);
                    cs.Save((ZCutscene*)newCs, "", &csWriter);

                    AddFile(fName, csStream->ToVector());
                }
            }
            else {
                Globals::Instance->GetSegmentedPtrName(cmdSetCutscenes->cmdArg2, room->parent, "CutsceneData", listName, res->parent->workerID);
                std::string fName = OTRExporter_DisplayList::GetPathToRes(room, listName);
                writer->Write(fName);

                MemoryStream* csStream = new MemoryStream();
                BinaryWriter csWriter = BinaryWriter(csStream);
                OTRExporter_Cutscene cs;
                cs.Save(cmdSetCutscenes->cutscenes[0], "", &csWriter);

                AddFile(fName, csStream->ToVector());
            }
        }
            break;
        case RoomCommand::SetPathways:
        {
            SetPathways* cmdSetPathways = (SetPathways*)cmd;

            writer->Write((uint32_t)cmdSetPathways->pathwayList.pathways.size());

            for (size_t i = 0; i < cmdSetPathways->pathwayList.pathways.size(); i++)
            {
                Declaration* decl = room->parent->GetDeclaration(GETSEGOFFSET(cmdSetPathways->pathwayList.pathways[i].listSegmentAddress));
                //std::string path = StringHelper::Sprintf("%s\\%s", OTRExporter_DisplayList::GetParentFolderName(res).c_str(), decl->varName.c_str());
                std::string path = OTRExporter_DisplayList::GetPathToRes(room, decl->declName);
                writer->Write(path);

                MemoryStream* pathStream = new MemoryStream();
                BinaryWriter pathWriter = BinaryWriter(pathStream);
                OTRExporter_Path pathExp;
                pathExp.Save(&cmdSetPathways->pathwayList, outPath, &pathWriter);

                AddFile(path, pathStream->ToVector());
            }
        }
            break;
        case RoomCommand::EndMarker:
            break;
        case RoomCommand::SetActorCutsceneList:
        {
            SetActorCutsceneList* list = (SetActorCutsceneList*)cmd;
            writer->Write((uint32_t)list->cutscenes.size());

            for (const auto& e : list->cutscenes) {
                writer->Write(e.priority);
                writer->Write(e.length);
                writer->Write(e.csCamId);
                writer->Write(e.scriptIndex);
                writer->Write(e.additionalCsId);
                writer->Write(e.endSfx);
                writer->Write(e.customValue);
                writer->Write(e.hudVisibility);
                writer->Write(e.endCam);
                writer->Write(e.letterboxSize);
            }
            break;
        }
#ifdef GAME_MM
        case RoomCommand::SetAnimatedMaterialList: {
            SetAnimatedMaterialList* list = (SetAnimatedMaterialList*)cmd;
            std::string listName;
            Globals::Instance->GetSegmentedPtrName(cmd->cmdArg2, cmd->parent, "AnimatedMaterial", listName,
                res->parent->workerID);
            listName = OTRExporter_DisplayList::GetPathToRes(room, listName);
            writer->Write(listName);

            MemoryStream* animatedMatStream = new MemoryStream();
            BinaryWriter animatedMatWriter = BinaryWriter(animatedMatStream);
            OTRExporter_TextureAnimation texAnim;
            
            texAnim.Save(&list->textureAnimation, outPath, &animatedMatWriter);

            AddFile(listName, animatedMatStream->ToVector());

            break;
        }
#endif
        case RoomCommand::SetMinimapList: {
            SetMinimapList* list = (SetMinimapList*)cmd;

            writer->Write((uint32_t)list->minimaps.size());
            writer->Write(list->scale);
            
            for (const auto& m : list->minimaps) {
                writer->Write(m.unk0);
                writer->Write(m.unk2);
                writer->Write(m.unk4);
                writer->Write(m.unk6);
                writer->Write(m.unk8);
            }

            break;
        }
        case RoomCommand::SetMinimapChests: {
            SetMinimapChests* chests = (SetMinimapChests*)cmd;

            writer->Write((uint32_t)chests->chests.size());

            for (const auto& c : chests->chests) {
                writer->Write(c.unk0);
                writer->Write(c.unk2);
                writer->Write(c.unk4);
                writer->Write(c.unk6);
                writer->Write(c.unk8);
            }
            break;
        }
        default:
            printf("UNIMPLEMENTED COMMAND: 0x%02X\n", (int)cmd->cmdID);

            break;
        }
    }
}

void OTRExporter_Room::WritePolyDList(BinaryWriter* writer, ZRoom* room, RoomShapeDListsEntry* dlist)
{
    writer->Write(dlist->polyType);

    switch (dlist->polyType)
    {
    case 2:
        writer->Write(dlist->x);
        writer->Write(dlist->y);
        writer->Write(dlist->z);
        writer->Write(dlist->unk_06);
        [[fallthrough]];
    default:
        //writer->Write(StringHelper::Sprintf("%s/%s", OTRExporter_DisplayList::GetParentFolderName(res).c_str(), dListDeclOpa->varName.c_str()));

        if (dlist->opaDList != nullptr)
        {
            auto opaDecl = room->parent->GetDeclaration(GETSEGOFFSET(dlist->opaDList->GetRawDataIndex()));
            writer->Write(StringHelper::Sprintf("%s/%s", OTRExporter_DisplayList::GetParentFolderName(room).c_str(), opaDecl->declName.c_str()));
        }
        else
            writer->Write("");

        if (dlist->xluDList != nullptr)
        {
            auto xluDecl = room->parent->GetDeclaration(GETSEGOFFSET(dlist->xluDList->GetRawDataIndex()));
            writer->Write(StringHelper::Sprintf("%s/%s", OTRExporter_DisplayList::GetParentFolderName(room).c_str(), xluDecl->declName.c_str()));
        }
        else
            writer->Write("");
        break;
    }
}
