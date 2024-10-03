#include "modding.h"
#include "better_double_sot.h"
#include "transform_ids.h"
#include "sys_cfb.h"
// #include "overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_scope.h"

extern s16 sTextboxWidth;
extern s16 sTextboxHeight;
// extern s16 sTextboxTexWidth;
// extern s16 sTextboxTexHeight;
extern u64 gOcarinaTrebleClefTex[];

// @recomp Patch textboxes to use ortho tris with a matrix so they can be interpolated.
RECOMP_PATCH void Message_DrawTextBox(PlayState* play, Gfx** gfxP) {
    MessageContext* msgCtx = &play->msgCtx;
    Gfx* gfx = *gfxP;

    gDPPipeSync(gfx++);

    if (((u32)msgCtx->textBoxType == TEXTBOX_TYPE_0) || (msgCtx->textBoxType == TEXTBOX_TYPE_2) ||
        (msgCtx->textBoxType == TEXTBOX_TYPE_9) || (msgCtx->textBoxType == TEXTBOX_TYPE_A)) {
        gDPSetRenderMode(gfx++, G_RM_CLD_SURF, G_RM_CLD_SURF2);
    } else if (msgCtx->textBoxType == TEXTBOX_TYPE_3) {
        gDPSetAlphaCompare(gfx++, G_AC_THRESHOLD);
        gDPSetRenderMode(gfx++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    }

    gDPSetPrimColor(gfx++, 0, 0, msgCtx->textboxColorRed, msgCtx->textboxColorGreen, msgCtx->textboxColorBlue,
                    msgCtx->textboxColorAlphaCurrent);

    if (((u32)msgCtx->textBoxType == TEXTBOX_TYPE_0) || (msgCtx->textBoxType == TEXTBOX_TYPE_2) ||
        (msgCtx->textBoxType == TEXTBOX_TYPE_6) || (msgCtx->textBoxType == TEXTBOX_TYPE_8) ||
        (msgCtx->textBoxType == TEXTBOX_TYPE_9) || (msgCtx->textBoxType == TEXTBOX_TYPE_A)) {
        gDPLoadTextureBlock_4b(gfx++, msgCtx->textboxSegment, G_IM_FMT_I, 128, 64, 0, G_TX_MIRROR | G_TX_WRAP,
                               G_TX_NOMIRROR | G_TX_WRAP, 7, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
    } else {
        gDPPipeSync(gfx++);

        if (msgCtx->textBoxType == TEXTBOX_TYPE_3) {
            gDPSetEnvColor(gfx++, 0, 0, 0, 255);
        } else if (msgCtx->textBoxType == TEXTBOX_TYPE_D) {
            gDPSetEnvColor(gfx++, 20, 0, 10, 255);
        } else {
            gDPSetEnvColor(gfx++, 50, 20, 0, 255);
        }
        gDPLoadTextureBlock_4b(gfx++, msgCtx->textboxSegment, G_IM_FMT_IA, 128, 64, 0, G_TX_MIRROR | G_TX_WRAP,
                               G_TX_MIRROR | G_TX_WRAP, 7, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
    }

    // @recomp Push the old RDP/RSP params.
    gEXPushProjectionMatrix(gfx++);
    gEXPushGeometryMode(gfx++);
    gEXPushOtherMode(gfx++);
    gEXPushViewport(gfx++);
    gEXMatrixGroupSimple(gfx++, TEXTBOX_TRANSFORM_PROJECTION_ID, G_EX_PUSH, G_MTX_PROJECTION,
        G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR, G_EX_EDIT_NONE);
    gEXMatrixGroupSimple(gfx++, TEXTBOX_TRANSFORM_ID, G_EX_PUSH, G_MTX_MODELVIEW,
        G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR, G_EX_EDIT_NONE);

    // @recomp Set up the RSP params.
    gSPLoadGeometryMode(gfx++, 0);
    gSPTexture(gfx++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    Vp* textbox_viewport = GRAPH_ALLOC(play->state.gfxCtx, sizeof(Vp));

    textbox_viewport->vp.vscale[0] = (gCfbWidth / 2) << 2;
    textbox_viewport->vp.vscale[1] = (gCfbHeight / 2) << 2;
    textbox_viewport->vp.vscale[2] = G_MAXZ;
    textbox_viewport->vp.vscale[3] = 0;

    textbox_viewport->vp.vtrans[0] = (gCfbWidth / 2) << 2;
    textbox_viewport->vp.vtrans[1] = (gCfbHeight / 2) << 2;
    textbox_viewport->vp.vtrans[2] = 0;
    textbox_viewport->vp.vtrans[3] = 0;

    gSPViewport(gfx++, textbox_viewport);

    if (msgCtx->textBoxType == TEXTBOX_TYPE_A) {
        gSPTextureRectangle(gfx++, msgCtx->textboxX << 2, (msgCtx->textboxY + 22) << 2,
                            (msgCtx->textboxX + msgCtx->unk12008) << 2, (msgCtx->textboxY + 54) << 2, G_TX_RENDERTILE,
                            0, 6, msgCtx->unk1200C << 1, 2 << 10);
    } else {
        const s32 base_textbox_width = 256;
        const s32 base_textbox_height = 64;

        // @recomp Calculate a scale based on the the target size derivatives.
        f32 textbox_scale_x = sTextboxWidth / (f32)base_textbox_width;
        f32 textbox_scale_y = sTextboxHeight / (f32)base_textbox_height;

        // @recomp Calculate the textbox center.
        f32 textbox_center_x = msgCtx->textboxX + sTextboxWidth / 2.0f;
        f32 textbox_center_y = msgCtx->textboxY + sTextboxHeight / 2.0f;

        // @recomp Allocate and build the matrices.
        Mtx* textbox_model_matrix = GRAPH_ALLOC(play->state.gfxCtx, sizeof(Mtx));
        Mtx* textbox_proj_matrix  = GRAPH_ALLOC(play->state.gfxCtx, sizeof(Mtx));
        guOrtho(textbox_proj_matrix, 0, gCfbWidth, gCfbHeight, 0, -1.0f, 1.0f, 1.0f);
        Mtx_SetTranslateScaleMtx(textbox_model_matrix, textbox_scale_x, textbox_scale_y, 1.0f, textbox_center_x, textbox_center_y, 0.0f);

        // @recomp Static vertex list for the textboxes.
        static Vtx textbox_verts[4] = {
            {{{-base_textbox_width / 2, -base_textbox_height / 2, 0}, 0, {                 0 << 5,                    0 << 5}, {0, 0, 0, 0xFF}}},
            {{{ base_textbox_width / 2, -base_textbox_height / 2, 0}, 0, {base_textbox_width << 5,                    0 << 5}, {0, 0, 0, 0xFF}}},
            {{{-base_textbox_width / 2,  base_textbox_height / 2, 0}, 0, {                 0 << 5,  base_textbox_height << 5}, {0, 0, 0, 0xFF}}},
            {{{ base_textbox_width / 2,  base_textbox_height / 2, 0}, 0, {base_textbox_width << 5,  base_textbox_height << 5}, {0, 0, 0, 0xFF}}},
        };

        // @recomp Loads the matrices, then the verts, and then draw the textbox.
        gSPMatrix(gfx++, textbox_model_matrix, G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gSPMatrix(gfx++, textbox_proj_matrix, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_PROJECTION);
        gSPVertex(gfx++, textbox_verts, 4, 0);

        // @recomp Use point filtering as this texture doesn't work well with bilerp. Also enable perspective correction for drawing tris.
        gDPSetTextureFilter(gfx++, G_TF_POINT);
        gDPSetTexturePersp(gfx++, G_TP_PERSP);
        gSP2Triangles(gfx++, 0, 1, 3, 0x0, 0, 3, 2, 0x0);

        // @recomp Restore bilerp filtering, disable perspective correction, and pop the model matrix
        gDPSetTextureFilter(gfx++, G_TF_BILERP);
        gDPSetTexturePersp(gfx++, G_TP_NONE);
        gSPPopMatrix(gfx++, G_MTX_MODELVIEW);
    }

    // @mod Draw DSoT hour selection clock.
    if (play->msgCtx.ocarinaMode == OCARINA_MODE_PROCESS_DOUBLE_TIME) {
        dsot_draw_clock(play);
    }

    // Draw treble clef
    if (msgCtx->textBoxType == TEXTBOX_TYPE_3) {
        gDPPipeSync(gfx++);
        gDPSetCombineLERP(gfx++, 1, 0, PRIMITIVE, 0, TEXEL0, 0, PRIMITIVE, 0, 1, 0, PRIMITIVE, 0, TEXEL0, 0, PRIMITIVE,
                          0);
        gDPSetPrimColor(gfx++, 0, 0, 255, 100, 0, 255);
        gDPLoadTextureBlock_4b(gfx++, gOcarinaTrebleClefTex, G_IM_FMT_I, 16, 32, 0, G_TX_MIRROR | G_TX_WRAP,
                               G_TX_MIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
        gSPTextureRectangle(gfx++, 78 << 2, 166 << 2, 94 << 2, 198 << 2, G_TX_RENDERTILE, 0, 0, 1 << 10, 1 << 10);
    }

    // @recomp Restore the old RDP/RSP params.
    gEXPopProjectionMatrix(gfx++);
    gEXPopGeometryMode(gfx++);
    gEXPopOtherMode(gfx++);
    gEXPopViewport(gfx++);
    gSPPopMatrix(gfx++, G_MTX_MODELVIEW);
    gEXPopMatrixGroup(gfx++, G_MTX_MODELVIEW);
    gEXPopMatrixGroup(gfx++, G_MTX_PROJECTION);
    gSPPerspNormalize(gfx++, play->view.perspNorm);

    *gfxP = gfx++;
}
