#include "CutsceneExporter.h"
#include "Globals.h"
#include <libultraship/bridge.h>

void OTRExporter_Cutscene::Save(ZResource* res, const fs::path& outPath, BinaryWriter* writer) {
    ZCutscene* cs = (ZCutscene*)res;

    WriteHeader(cs, outPath, writer, static_cast<uint32_t>(SOH::ResourceType::SOH_Cutscene));

    writer->Write((uint32_t)0);

    const auto currentStream = writer->GetBaseAddress();

    writer->Write(CS_BEGIN_CUTSCENE(cs->numCommands, cs->endFrame));

    if (Globals::Instance->game == ZGame::MM_RETAIL) {
        SaveMM(cs, writer);
    } else {
        SaveOot(cs, writer);
    }


    // CS_END
    writer->Write(0xFFFFFFFF);
    writer->Write((uint32_t)0);

    const auto endStream = writer->GetBaseAddress();
    writer->Seek((uint32_t)currentStream - 4, SeekOffsetType::Start);
    writer->Write((uint32_t)((endStream - currentStream) / 4));
    writer->Seek((uint32_t)endStream, SeekOffsetType::Start);
}

void OTRExporter_Cutscene::SaveOot(ZCutscene* cs, BinaryWriter* writer) {

    for (size_t i = 0; i < cs->commands.size(); i++) {
        switch (cs->commands[i]->commandID) {
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_CAM_EYE_SPLINE: {
                CutsceneOoTCommand_GenericCameraCmd* cmdCamPos = (CutsceneOoTCommand_GenericCameraCmd*)cs->commands[i];

                writer->Write(CS_CMD_CAM_EYE);
                writer->Write(CMD_HH(0x0001, ((CutsceneOoTCommand_GenericCameraCmd*)cs->commands[i])->startFrame));
                writer->Write(CMD_HH(cmdCamPos->endFrame, 0x0000));

                for (const auto& e : cs->commands[i]->entries) {
                    CutsceneOoTCommand_CameraPoint* point = (CutsceneOoTCommand_CameraPoint*)e;
                    writer->Write(CMD_BBH(point->continueFlag, point->cameraRoll, point->nextPointFrame));
                    writer->Write(point->viewAngle);
                    writer->Write(CMD_HH(point->posX, point->posY));
                    writer->Write(CMD_HH(point->posZ, point->unused));
                }
            } break;
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_CAM_AT_SPLINE: {
                CutsceneOoTCommand_GenericCameraCmd* cmdCamPos = (CutsceneOoTCommand_GenericCameraCmd*)cs->commands[i];

                writer->Write(CS_CMD_CAM_AT);
                writer->Write(CMD_HH(0x0001, cmdCamPos->startFrame));
                writer->Write(CMD_HH(cmdCamPos->endFrame, 0x0000));

                for (const auto& e : cs->commands[i]->entries) {
                    CutsceneOoTCommand_CameraPoint* point = (CutsceneOoTCommand_CameraPoint*)e;
                    writer->Write(CMD_BBH(point->continueFlag, point->cameraRoll, point->nextPointFrame));
                    writer->Write(point->viewAngle);
                    writer->Write(CMD_HH(point->posX, point->posY));
                    writer->Write(CMD_HH(point->posZ, point->unused));
                }
                break;
            }
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_MISC: {
                writer->Write(CS_CMD_MISC);
                writer->Write((uint32_t)CMD_W((cs->commands[i])->entries.size()));
                for (const auto& e : cs->commands[i]->entries) // All in OOT seem to only have 1 entry
                {
                    CutsceneOoTSubCommandEntry_GenericCmd* cmd = (CutsceneOoTSubCommandEntry_GenericCmd*)e;
                    writer->Write(CMD_HH(cmd->base, cmd->startFrame));
                    writer->Write(CMD_HH(cmd->endFrame, cmd->pad));
                    writer->Write(CMD_W(cmd->unused1));
                    writer->Write(CMD_W(cmd->unused2));
                    writer->Write(CMD_W(cmd->unused3));
                    writer->Write(CMD_W(cmd->unused4));
                    writer->Write(CMD_W(cmd->unused5));
                    writer->Write(CMD_W(cmd->unused6));
                    writer->Write(CMD_W(cmd->unused7));
                    writer->Write(CMD_W(cmd->unused8));
                    writer->Write(CMD_W(cmd->unused9));
                    writer->Write(CMD_W(cmd->unused10));
                }
                break;
            }
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_LIGHT_SETTING: {
                writer->Write(CS_CMD_SET_LIGHTING);
                writer->Write((uint32_t)CMD_W((cs->commands[i])->entries.size()));
                for (const auto& e : cs->commands[i]->entries) {
                    CutsceneOoTSubCommandEntry_GenericCmd* cmd = (CutsceneOoTSubCommandEntry_GenericCmd*)e;
                    writer->Write(CMD_HH(cmd->base, cmd->startFrame));
                    writer->Write(CMD_HH(cmd->endFrame, cmd->pad));
                    writer->Write(CMD_W(cmd->unused1));
                    writer->Write(CMD_W(cmd->unused2));
                    writer->Write(CMD_W(cmd->unused3));
                    writer->Write(CMD_W(cmd->unused4));
                    writer->Write(CMD_W(cmd->unused5));
                    writer->Write(CMD_W(cmd->unused6));
                    writer->Write(CMD_W(cmd->unused7));
                    writer->Write((uint32_t)0x0);
                    writer->Write((uint32_t)0x0);
                    writer->Write((uint32_t)0x0);
                }
                break;
            }
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_CAM_EYE_SPLINE_REL_TO_PLAYER: {
                CutsceneOoTCommand_GenericCameraCmd* cmdCamPos = (CutsceneOoTCommand_GenericCameraCmd*)cs->commands[i];

                writer->Write(CS_CMD_CAM_EYE_REL_TO_PLAYER);
                writer->Write(CMD_HH(0x0001, cmdCamPos->startFrame));
                writer->Write(CMD_HH(cmdCamPos->endFrame, 0x0000));

                for (const auto& e : cs->commands[i]->entries) {
                    CutsceneOoTCommand_CameraPoint* point = (CutsceneOoTCommand_CameraPoint*)e;
                    writer->Write(CMD_BBH(point->continueFlag, point->cameraRoll, point->nextPointFrame));
                    writer->Write(point->viewAngle);
                    writer->Write(CMD_HH(point->posX, point->posY));
                    writer->Write(CMD_HH(point->posZ, point->unused));
                }
                break;
            }
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_CAM_AT_SPLINE_REL_TO_PLAYER: {
                CutsceneOoTCommand_GenericCameraCmd* cmdCamPos = (CutsceneOoTCommand_GenericCameraCmd*)cs->commands[i];

                writer->Write(CS_CMD_CAM_AT_REL_TO_PLAYER);
                writer->Write(CMD_HH(0x0001, cmdCamPos->startFrame));
                writer->Write(CMD_HH(cmdCamPos->endFrame, 0x0000));

                for (const auto& e : cs->commands[i]->entries) {
                    CutsceneOoTCommand_CameraPoint* point = (CutsceneOoTCommand_CameraPoint*)e;
                    writer->Write(CMD_BBH(point->continueFlag, point->cameraRoll, point->nextPointFrame));
                    writer->Write(point->viewAngle);
                    writer->Write(CMD_HH(point->posX, point->posY));
                    writer->Write(CMD_HH(point->posZ, point->unused));
                }
                break;
            }

            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_CAM_EYE: // Not used in OOT
                break;
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_CAM_AT: // Not used in OOT
                break;

            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_RUMBLE_CONTROLLER: {
                writer->Write(CS_CMD_09);
                writer->Write((uint32_t)CMD_W(((CutsceneOoTCommand_Rumble*)cs->commands[i])->entries.size()));

                for (const auto& e : cs->commands[i]->entries) {
                    CutsceneOoTSubCommandEntry_Rumble* r = (CutsceneOoTSubCommandEntry_Rumble*)e;
                    writer->Write(CMD_HH(r->base, r->startFrame));
                    writer->Write(CMD_HBB(e->endFrame, r->sourceStrength, r->duration));
                    writer->Write(CMD_BBH(r->decreaseRate, r->unk_09, r->unk_0A));
                }
                break;
            }
                // case 0x15://Both unused in OoT
                // case 0x1A://(uint32_t)CutsceneOoT_CommandType::Unknown:
                {
#if 0
            CutsceneCommandUnknown* cmdUnk = (CutsceneCommandUnknown*)cs->commands[i];
            writer->Write((uint32_t)cs->commands[i]->commandID);
            writer->Write((uint32_t)cmdUnk->entries.size());

            for (const auto e : cmdUnk->entries)
            {
                writer->Write(CMD_W(e->unused0));
                writer->Write(CMD_W(e->unused1));
                writer->Write(CMD_W(e->unused2));
                writer->Write(CMD_W(e->unused3));
                writer->Write(CMD_W(e->unused4));
                writer->Write(CMD_W(e->unused5));
                writer->Write(CMD_W(e->unused6));
                writer->Write(CMD_W(e->unused7));
                writer->Write(CMD_W(e->unused8));
                writer->Write(CMD_W(e->unused9));
                writer->Write(CMD_W(e->unused10));
                writer->Write(CMD_W(e->unused11));
            }
#endif
                }
                break;
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_TEXT: {
                writer->Write(CS_CMD_TEXTBOX);
                writer->Write((uint32_t)CMD_W((cs->commands[i])->entries.size()));

                for (const auto& e : cs->commands[i]->entries) {
                    CutsceneOoTSubCommandEntry_Text* textBox = (CutsceneOoTSubCommandEntry_Text*)e;
                    if (textBox->base == 0xFFFF) // CS_TEXT_NONE
                    {
                        writer->Write(CMD_HH(0xFFFF, textBox->startFrame));
                        writer->Write(CMD_HH(textBox->endFrame, 0xFFFF));
                        writer->Write(CMD_HH(0xFFFF, 0xFFFF));
                    } else // CS_TEXT_DISPLAY_TEXTBOX
                    {
                        writer->Write(CMD_HH(textBox->base, textBox->startFrame));
                        writer->Write(CMD_HH(textBox->endFrame, textBox->type));
                        writer->Write(CMD_HH(textBox->textId1, textBox->textId2));
                    }
                }
                break;
            }
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_PLAYER_CUE: // ActorAction0
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_0_0:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_0_1:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_0_2:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_0_3:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_0_4:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_0_5:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_0_6:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_0_7:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_0_8:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_0_9:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_0_10:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_0_11:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_0_12:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_0_13:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_0_14:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_0_15:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_0_16:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_0_17:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_1_0:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_1_1:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_1_2:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_1_3:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_1_4:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_1_5:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_1_6:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_1_7:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_1_8:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_1_9:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_1_10:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_1_11:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_1_12:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_1_13:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_1_14:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_1_15:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_1_16:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_1_17:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_2_0:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_2_1:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_2_2:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_2_3:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_2_4:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_2_5:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_2_6:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_2_7:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_2_8:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_2_9:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_2_10:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_2_11:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_2_12:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_2_13:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_3_0:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_3_1:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_3_2:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_3_3:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_3_4:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_3_5:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_3_6:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_3_7:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_3_8:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_3_9:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_3_10:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_3_11:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_3_12:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_4_0:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_4_1:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_4_2:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_4_3:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_4_4:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_4_5:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_4_6:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_4_7:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_4_8:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_5_0:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_5_1:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_5_2:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_5_3:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_5_4:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_5_5:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_5_6:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_6_0:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_6_1:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_6_2:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_6_3:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_6_4:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_6_5:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_6_6:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_6_7:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_7_0:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_7_1:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_7_2:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_7_3:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_7_4:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_7_5:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_7_6:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_8_0:
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_ACTOR_CUE_9_0: {
                writer->Write((uint32_t)cs->commands[i]->commandID);
                writer->Write((uint32_t)CMD_W(cs->commands[i]->entries.size()));

                for (const auto& e : cs->commands[i]->entries) {
                    CutsceneOoTSubCommandEntry_ActorCue* actorAct = (CutsceneOoTSubCommandEntry_ActorCue*)e;
                    writer->Write(CMD_HH(actorAct->base, actorAct->startFrame));
                    writer->Write(CMD_HH(actorAct->endFrame, actorAct->rotX));
                    writer->Write(CMD_HH(actorAct->rotY, actorAct->rotZ));
                    writer->Write(CMD_W(actorAct->startPosX));
                    writer->Write(CMD_W(actorAct->startPosY));
                    writer->Write(CMD_W(actorAct->startPosZ));
                    writer->Write(CMD_W(actorAct->endPosX));
                    writer->Write(CMD_W(actorAct->endPosY));
                    writer->Write(CMD_W(actorAct->endPosZ));
                    writer->Write(CMD_W(actorAct->normalX));
                    writer->Write(CMD_W(actorAct->normalY));
                    writer->Write(CMD_W(actorAct->normalZ));
                }

                break;
            }
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_TRANSITION: {
                CutsceneOoTCommand_Transition* cmdTFX = (CutsceneOoTCommand_Transition*)cs->commands[i];

                writer->Write(CS_CMD_SCENE_TRANS_FX);
                writer->Write((uint32_t)1);
                writer->Write(CMD_HH((cmdTFX->base), cmdTFX->startFrame));
                writer->Write(CMD_HH((cmdTFX->endFrame), cmdTFX->endFrame));
                break;
            }
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_START_SEQ: {
                writer->Write(CS_CMD_PLAYBGM);
                writer->Write((uint32_t)CMD_W(cs->commands[i]->entries.size()));

                for (const auto& e : cs->commands[i]->entries) {
                    CutsceneOoTSubCommandEntry_GenericCmd* cmd = (CutsceneOoTSubCommandEntry_GenericCmd*)e;
                    writer->Write(CMD_HH(cmd->base, cmd->startFrame));
                    writer->Write(CMD_HH(cmd->endFrame, cmd->pad));
                    writer->Write(CMD_W(cmd->unused1));
                    writer->Write(CMD_W(cmd->unused2));
                    writer->Write(CMD_W(cmd->unused3));
                    writer->Write(CMD_W(cmd->unused4));
                    writer->Write(CMD_W(cmd->unused5));
                    writer->Write(CMD_W(cmd->unused6));
                    writer->Write(CMD_W(cmd->unused7));
                    writer->Write((uint32_t)0);
                    writer->Write((uint32_t)0);
                    writer->Write((uint32_t)0);
                }
                break;
            }
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_STOP_SEQ: {
                writer->Write(CS_CMD_STOPBGM);
                writer->Write((uint32_t)CMD_W(cs->commands[i]->entries.size()));

                for (const auto& e : cs->commands[i]->entries) {
                    CutsceneOoTSubCommandEntry_GenericCmd* cmd = (CutsceneOoTSubCommandEntry_GenericCmd*)e;
                    writer->Write(CMD_HH(cmd->base, cmd->startFrame));
                    writer->Write(CMD_HH(cmd->endFrame, cmd->pad));
                    writer->Write(CMD_W(cmd->unused1));
                    writer->Write(CMD_W(cmd->unused2));
                    writer->Write(CMD_W(cmd->unused3));
                    writer->Write(CMD_W(cmd->unused4));
                    writer->Write(CMD_W(cmd->unused5));
                    writer->Write(CMD_W(cmd->unused6));
                    writer->Write(CMD_W(cmd->unused7));
                    writer->Write((uint32_t)0);
                    writer->Write((uint32_t)0);
                    writer->Write((uint32_t)0);
                }
                break;
            }
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_FADE_OUT_SEQ: {
                writer->Write(CS_CMD_FADEBGM);
                writer->Write((uint32_t)CMD_W(cs->commands[i]->entries.size()));

                for (const auto& e : cs->commands[i]->entries) {
                    CutsceneOoTSubCommandEntry_GenericCmd* cmd = (CutsceneOoTSubCommandEntry_GenericCmd*)e;
                    writer->Write(CMD_HH(cmd->base, cmd->startFrame));
                    writer->Write(CMD_HH(cmd->endFrame, cmd->pad));
                    writer->Write(CMD_W(cmd->unused1));
                    writer->Write(CMD_W(cmd->unused2));
                    writer->Write(CMD_W(cmd->unused3));
                    writer->Write(CMD_W(cmd->unused4));
                    writer->Write(CMD_W(cmd->unused5));
                    writer->Write(CMD_W(cmd->unused6));
                    writer->Write(CMD_W(cmd->unused7));
                    writer->Write((uint32_t)0);
                    writer->Write((uint32_t)0);
                    writer->Write((uint32_t)0);
                }
                break;
            }
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_TIME: {
                writer->Write(CS_CMD_SETTIME);
                writer->Write((uint32_t)CMD_W(cs->commands[i]->entries.size()));

                for (const auto& e : cs->commands[i]->entries) {
                    CutsceneSubCommandEntry_SetTime* t = (CutsceneSubCommandEntry_SetTime*)e;
                    writer->Write(CMD_HH(t->base, t->startFrame));
                    writer->Write(CMD_HBB(t->endFrame, t->hour, t->minute));
                    writer->Write((uint32_t)0);
                    // writer->Write((uint32_t)CMD_W(t->unk_08));
                }
                break;
            }
            case (uint32_t)CutsceneOoT_CommandType::CS_CMD_DESTINATION: {
                CutsceneOoTCommand_Destination* t = (CutsceneOoTCommand_Destination*)cs->commands[i];
                writer->Write(CS_CMD_TERMINATOR);
                writer->Write((uint32_t)1);
                writer->Write(CMD_HH(t->base, t->startFrame));
                writer->Write(CMD_HH(t->endFrame, t->endFrame));
                break;
            }
            default: {
                // writer->Write((uint32_t)cs->commands[i]->commandID);
                printf("Undefined CS Opcode: %04X\n", cs->commands[i]->commandID);
            } break;
        }
    }
}
// MUST be uint16_t for -1
typedef enum : uint16_t {
    /* -1 */ CS_TEXT_TYPE_NONE = (uint16_t)-1,
    /*  0 */ CS_TEXT_TYPE_DEFAULT = 0,
    /*  1 */ CS_TEXT_TYPE_1,
    /*  2 */ CS_TEXT_OCARINA_ACTION,
    /*  3 */ CS_TEXT_TYPE_3,
    /*  4 */ CS_TEXT_TYPE_BOSSES_REMAINS, // use `altText1` in the Giant Cutscene if all remains are already obtained
    /*  5 */ CS_TEXT_TYPE_ALL_NORMAL_MASKS
} CutsceneTextType;

typedef enum : uint8_t {
    /* 0 */ Header,
    /* 1 */ CamPoint,
    /* 2 */ CamMisc,
    /* 3 */ Footer,
} CutsceneSplineType;

void OTRExporter_Cutscene::SaveMM(ZCutscene* cs, BinaryWriter* writer) {
    for (size_t i = 0; i < cs->commands.size(); i++) {
        writer->Write((uint32_t)cs->commands[i]->commandID);

        if (((cs->commands[i]->commandID >= (uint32_t)CutsceneMM_CommandType::CS_CMD_ACTOR_CUE_100) &&
             (cs->commands[i]->commandID <= (uint32_t)CutsceneMM_CommandType::CS_CMD_ACTOR_CUE_149)) ||
            (cs->commands[i]->commandID == (uint32_t)CutsceneMM_CommandType::CS_CMD_ACTOR_CUE_201) ||
            ((cs->commands[i]->commandID >= (uint32_t)CutsceneMM_CommandType::CS_CMD_ACTOR_CUE_450) &&
             (cs->commands[i]->commandID <= (uint32_t)CutsceneMM_CommandType::CS_CMD_ACTOR_CUE_599))) {
            goto actorCue;
        }

        switch ((CutsceneMM_CommandType)cs->commands[i]->commandID) {
            case CutsceneMM_CommandType::CS_CMD_TEXT: {
                writer->Write((uint32_t)cs->commands[i]->entries.size());
                for (const auto& e : cs->commands[i]->entries) {
                    auto* cmd = (CutsceneMMSubCommandEntry_Text*)e;

                    writer->Write(CMD_HH(cmd->base, cmd->startFrame));
                    writer->Write(CMD_HH(cmd->endFrame, cmd->type));
                    writer->Write(CMD_HH(cmd->textId1, cmd->textId2));
                }
                break;
            }

            // BENTODO: Can these stay consilidated?
            case CutsceneMM_CommandType::CS_CMD_CAMERA_SPLINE: {
                // This command uses 4 different macros that are all different sizes and uses the number of bytes of the 
                // whole command instead of entries like most other commands. numEntries isn't actually the number of entries in the command
                // rather the number of bytes the command will take up in the rom. We also can not simply use GetCommandSize because it returns
                // a larger size to help ZAPD know where to start reading for the next command.
                writer->Write((uint32_t)cs->commands[i]->numEntries);
                // monkaS
                size_t j = 0;
                for (;;) {
                    auto* header = dynamic_cast<CutsceneSubCommandEntry_SplineHeader*>(cs->commands[i]->entries[j]);

                    //Debugging check, can probably remove after testing
                    assert(header != nullptr);
                    uint32_t numEntries = header->numEntries;
                    writer->Write(CMD_HH(header->numEntries, header->unused0));
                    writer->Write(CMD_HH(header->unused1, header->duration));

                    for (size_t k = 0; k < numEntries * 2; k++) {
                        auto* element =
                            dynamic_cast<CutsceneSubCommandEntry_SplineCamPoint*>(cs->commands[i]->entries[++j]); 
                        // Debugging check, can probably remove after testing
                        assert(element != nullptr);
                        writer->Write(CMD_BBH(element->interpType, element->weight, element->duration));
                        writer->Write(CMD_HH(element->posX, element->posY));
                        writer->Write(CMD_HH(element->posZ, element->relTo));
                    }
                    for (size_t k = 0; k < numEntries; k++) {
                        auto* element =
                            dynamic_cast<CutsceneSubCommandEntry_SplineMiscPoint*>(cs->commands[i]->entries[++j]);
                        // Debugging check, can probably remove after testing
                        assert(element != nullptr);
                        writer->Write(CMD_HH(element->unused0, element->roll));
                        writer->Write(CMD_HH(element->fov, element->unused1));
                    }

                    auto* header2 = dynamic_cast<CutsceneSubCommandEntry_SplineFooter*>(cs->commands[i]->entries[++j]);
                    if (header2 != nullptr) {
                        if (header2->base == 0xFFFF) {
                            break;
                        }
                    }
                }
                
                // The footer exists as an error checking type and a way to output an empty macro. It holds no actual data.
                auto* footer = dynamic_cast<CutsceneSubCommandEntry_SplineFooter*>(cs->commands[i]->entries[j]);
                assert(footer != nullptr);
                writer->Write((uint16_t)0xFFFF);
                writer->Write((uint16_t)0x0004);
                //i += j;
                break;
            }
            case CutsceneMM_CommandType::CS_CMD_MISC:
            case CutsceneMM_CommandType::CS_CMD_LIGHT_SETTING:
            case CutsceneMM_CommandType::CS_CMD_TRANSITION:
            case CutsceneMM_CommandType::CS_CMD_MOTION_BLUR:
            case CutsceneMM_CommandType::CS_CMD_GIVE_TATL:
            case CutsceneMM_CommandType::CS_CMD_STOP_SEQ:
            case CutsceneMM_CommandType::CS_CMD_START_SEQ:
            case CutsceneMM_CommandType::CS_CMD_SFX_REVERB_INDEX_2:
            case CutsceneMM_CommandType::CS_CMD_SFX_REVERB_INDEX_1:
            case CutsceneMM_CommandType::CS_CMD_MODIFY_SEQ:
            case CutsceneMM_CommandType::CS_CMD_START_AMBIENCE:
            case CutsceneMM_CommandType::CS_CMD_FADE_OUT_AMBIENCE:
            case CutsceneMM_CommandType::CS_CMD_DESTINATION:
            case CutsceneMM_CommandType::CS_CMD_CHOOSE_CREDITS_SCENES:
            default:
            {
                writer->Write((uint32_t)cs->commands[i]->entries.size());
                for (const auto e : cs->commands[i]->entries) {
                    writer->Write(CMD_HH(e->base, e->startFrame));
                    writer->Write(CMD_HH(e->endFrame, e->pad));
                }
                break;
            }
            case CutsceneMM_CommandType::CS_CMD_TRANSITION_GENERAL: {
                writer->Write((uint32_t)cs->commands[i]->entries.size());
                for (const auto e : cs->commands[i]->entries) {
                    auto* cmd = (CutsceneSubCommandEntry_TransitionGeneral*)e;

                    writer->Write(CMD_HH(cmd->base, cmd->startFrame));
                    writer->Write(CMD_HBB(cmd->endFrame, cmd->unk_06, cmd->unk_07));
                    writer->Write(CMD_BBBB(cmd->unk_08, 0, 0, 0));
                }
                break;
            }
            case CutsceneMM_CommandType::CS_CMD_FADE_OUT_SEQ: {
                writer->Write((uint32_t)cs->commands[i]->entries.size());
                for (const auto e : cs->commands[i]->entries) {
                    writer->Write(CMD_HH(e->base, e->startFrame));
                    writer->Write(CMD_HH(e->endFrame, e->pad));
                    writer->Write(CMD_W(0));
                }
                break;
            }
            case CutsceneMM_CommandType::CS_CMD_TIME: {
                writer->Write((uint32_t)cs->commands[i]->entries.size());
                for (const auto e : cs->commands[i]->entries) {
                    auto* cmd = (CutsceneSubCommandEntry_SetTime*)e;

                    writer->Write(CMD_HH(cmd->base, cmd->startFrame));
                    writer->Write(CMD_HBB(cmd->endFrame, cmd->hour, cmd->minute));
                    writer->Write(CMD_W(0));
                }
                break;
            }
            case CutsceneMM_CommandType::CS_CMD_PLAYER_CUE: {
            actorCue:
                writer->Write((uint32_t)cs->commands[i]->entries.size());
                for (const auto e : cs->commands[i]->entries) {
                    auto* cmd = (CutsceneMMSubCommandEntry_ActorCue*)e;

                    writer->Write(CMD_HH(cmd->base, cmd->startFrame));
                    writer->Write(CMD_HH(cmd->endFrame, cmd->rotX));
                    writer->Write(CMD_HH(cmd->rotY, cmd->rotZ));
                    writer->Write(CMD_W(cmd->startPosX));
                    writer->Write(CMD_W(cmd->startPosY));
                    writer->Write(CMD_W(cmd->startPosZ));
                    writer->Write(CMD_W(cmd->endPosX));
                    writer->Write(CMD_W(cmd->endPosY));
                    writer->Write(CMD_W(cmd->endPosZ));
                    writer->Write(CMD_F(cmd->normalX));
                    writer->Write(CMD_F(cmd->normalY));
                    writer->Write(CMD_F(cmd->normalZ));
                }
                break;
            }
            case CutsceneMM_CommandType::CS_CMD_RUMBLE: {
                writer->Write((uint32_t)cs->commands[i]->entries.size());
                for (const auto e : cs->commands[i]->entries) {
                    auto* cmd = (CutsceneMMSubCommandEntry_Rumble*)e;

                    writer->Write(CMD_HH(cmd->base, cmd->startFrame));
                    writer->Write(CMD_HBB(cmd->endFrame, cmd->intensity, cmd->decayTimer));
                    writer->Write(CMD_BBBB(cmd->decayStep, 0, 0, 0));
                }
                break;
            }
        }
    }
}
