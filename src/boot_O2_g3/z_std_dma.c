#include <ultra64.h>
#include <global.h>

s32 Dmamgr_DoDmaTransfer(u32 a0, void* a1, u32 a2) {
    u32 spPad;
    OSIoMesg sp60;
    OSMesgQueue sp48;
    OSMesg sp44;
    s32 ret;
    u32 s0 = dmamgrChunkSize;

    osInvalDCache(a1, a2);
    osCreateMesgQueue(&sp48, &sp44, 1);

    if (s0 != 0) {
        while (s0 < a2) {
            sp60.hdr.pri = 0;
            sp60.hdr.retQueue = &sp48;
            sp60.devAddr = (u32)a0;
            sp60.dramAddr = a1;
            sp60.size = s0;
            ret = osEPiStartDma(D_80096B40, &sp60, 0);
            if (ret) goto END;

            osRecvMesg(&sp48, NULL, 1);
            a2 -= s0;
            a0 = a0 + s0;
            a1 = (u8*)a1 + s0;
        }
    }
    sp60.hdr.pri = 0;
    sp60.hdr.retQueue = &sp48;
    sp60.devAddr = (u32)a0;
    sp60.dramAddr = a1;
    sp60.size = (u32)a2;
    ret = osEPiStartDma(D_80096B40, &sp60, 0);
    if (ret) goto END;

    osRecvMesg(&sp48, NULL, 1);

    osInvalDCache(a1, a2);

END:
    return ret;
}

void Dmamgr_osEPiStartDmaWrapper(OSPiHandle* pihandle, OSIoMesg* mb, s32 direction) {
    osEPiStartDma(pihandle, mb, direction);
}

DmadataEntry* Dmamgr_FindDmaEntry(u32 a0) {
    DmadataEntry* curr;

    for (curr = dmadata; curr->vromEnd != 0; curr++) {
        if (a0 < curr->vromStart) continue;
        if (a0 >= curr->vromEnd) continue;

        return curr;
    }

    return NULL;
}

u32 Dmamgr_TranslateVromToRom(u32 a0) {
    DmadataEntry* v0 = Dmamgr_FindDmaEntry(a0);

    if (v0 != NULL) {
        if (v0->romEnd == 0) {
            return a0 + v0->romStart - v0->vromStart;
        }

        if (a0 == v0->vromStart) {
            return v0->romStart;
        }

        return -1;
    }

    return -1;
}

s32 Dmamgr_FindDmaIndex(u32 a0) {
    DmadataEntry* v0 = Dmamgr_FindDmaEntry(a0);

    if (v0 != NULL) {
		return v0 - dmadata;
    }

    return -1;
}

// TODO this should be a string
char* func_800809F4(u32 a0) {
    return &D_800981C0[0];
}

#ifdef NONMATCHING

void Dmamgr_HandleRequest(DmaRequest* a0) {
    UNK_TYPE sp34;
    UNK_TYPE sp30;
    UNK_TYPE sp2C;
    UNK_TYPE sp28;
    UNK_TYPE sp24;
    UNK_TYPE sp20;
    s32 sp1C;
    UNK_TYPE sp18;

    sp34 = (UNK_TYPE)a0->unk0;
    sp30 = (UNK_TYPE)a0->unk4;
    sp2C = a0->unk8;

    sp1C = Dmamgr_FindDmaIndex(sp34);

    if ((sp1C >= 0) && (sp1C < numDmaEntries)) {
        if (dmadata[sp1C].romEnd == 0) {
            if (dmadata[sp1C].vromEnd < (sp2C + sp34)) {
                func_80083E4C(&dmamgrString800981C4, 499);
            }
            Dmamgr_DoDmaTransfer((u8*)((dmadata[sp1C].romStart + sp34) - dmadata[sp1C].vromStart), (u8*)sp30, sp2C);
            return;
        }

        // TODO this part is arranged slightly different is ASM
        sp24 = dmadata[sp1C].romEnd - dmadata[sp1C].romStart;
        sp28 = dmadata[sp1C].romStart;

        if (sp34 != dmadata[sp1C].vromStart) {
            func_80083E4C(&dmamgrString800981D4, 518);
        }

        if (sp2C != (dmadata[sp1C].vromEnd - dmadata[sp1C].vromStart)) {
            func_80083E4C(&dmamgrString800981E4, 525);
        }

        osSetThreadPri(NULL, 10);
        Yaz0_LoadAndDecompressFile(sp28, sp30, sp24);
        osSetThreadPri(NULL, 17);
    } else {
        func_80083E4C(&dmamgrString800981F4, 558);
    }
}

#else

GLOBAL_ASM("./asm/nonmatching/z_std_dma/Dmamgr_HandleRequest.asm")

#endif

#ifdef NONMATCHING

void Dmamgr_ThreadEntry(void* a0) {
    DmaRequest* sp34;
	UNK_TYPE pad;
    DmaRequest* s0;

    for (;;) {
        osRecvMesg(&dmamgrMsq, (OSMesg)&sp34, 1);
        if (sp34 == NULL) return;
        s0 = sp34;
        Dmamgr_HandleRequest(s0);
        // TODO a0 isn't being used for this comparison
        if (s0->unk18 == NULL) continue;
        osSendMesg(&dmamgrMsq, (OSMesg)s0->unk1C, 0);
    }
}

#else

GLOBAL_ASM("./asm/nonmatching/z_std_dma/Dmamgr_ThreadEntry.asm")

#endif

#ifdef NONMATCHING

s32 Dmamgr_SendRequest(DmaRequest* a0, UNK_FUN_PTR(a1), UNK_PTR a2, UNK_TYPE a3, UNK_TYPE sp30, OSMesgQueue* sp34, UNK_TYPE sp38) {
    // TODO this isn't correct, it uses a lui, addiu to get the address of prenmiStage, then loads it,
	// meaning that this is likely just "if (*prenmiStage >= 2)". However, I can not get it to not
	// produce the usual lui, lw combo to load from an address :/
    if (*prenmiStage >= 2) {
        return -2;
    }

    a0->unk0 = a2;
    a0->unk4 = a1;
    a0->unk8 = a3;
    a0->unk14 = 0;
    a0->unk18 = sp34;
    a0->unk1C = sp38;

    osSendMesg(&dmamgrMsq, (OSMesg)a0, 1);

    return 0;
}

#else

GLOBAL_ASM("./asm/nonmatching/z_std_dma/Dmamgr_SendRequest.asm")

#endif

s32 Dmamgr_SendRequestAndWait(u32 a0, u32 a1, u32 a2) {
	DmaRequest sp48;
    OSMesgQueue sp30;
    OSMesg sp2C;
	s32 ret;

    osCreateMesgQueue(&sp30, &sp2C, 1);

	ret = Dmamgr_SendRequest(&sp48, a0, a1, a2, 0, &sp30, 0);

	if (ret == -1) {
		return ret;
	} else {
		osRecvMesg(&sp30, NULL, 1);
	}

	return 0;
}

#ifdef NONMATCHING

void Dmamgr_Start() {
	DmadataEntry* v0;
	u32 v1;
	// TODO register load ordering is wrong
	Dmamgr_DoDmaTransfer(&dmadata_vrom_start, dmadata, (u8*)&dmadata_vrom_end - (u8*)&dmadata_vrom_start);

	for (v0 = dmadata, v1 = 0; v0->vromEnd != 0; v0++, v1++);

	numDmaEntries = (u16)v1;

	osCreateMesgQueue(&dmamgrMsq, (OSMesg)&dmamgrMsqMessages, 32);

	thread_info_init(&dmamgrThreadInfo, &dmamgrStack, &D_8009BA08, 0, 256, &dmamgrThreadName);

	osCreateThread(&dmamgrOSThread, 18, Dmamgr_ThreadEntry, NULL, &D_8009BA08, 17);

	osStartThread(&dmamgrOSThread);
}

#else

GLOBAL_ASM("./asm/nonmatching/z_std_dma/Dmamgr_Start.asm")

#endif

void Dmamgr_Stop() {
    osSendMesg(&dmamgrMsq, NULL, 1);
}
