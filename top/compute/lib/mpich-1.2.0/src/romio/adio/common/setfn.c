/* 
 *   $Id: setfn.c,v 1.1 2000/05/10 21:42:48 jrjohns Exp $    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "adio.h"

void ADIOI_SetFunctions(ADIO_File fd)
{
    fd->fns = (ADIOI_Fns *) ADIOI_Malloc(sizeof(ADIOI_Fns));
    switch(fd->file_system) {
    case ADIO_PFS:
#ifdef __PFS	
	fd->fns->ADIOI_xxx_Open = ADIOI_PFS_Open;
	fd->fns->ADIOI_xxx_ReadContig = ADIOI_PFS_ReadContig;
	fd->fns->ADIOI_xxx_WriteContig = ADIOI_PFS_WriteContig;
	fd->fns->ADIOI_xxx_ReadStridedColl = ADIOI_PFS_ReadStridedColl;
	fd->fns->ADIOI_xxx_WriteStridedColl = ADIOI_PFS_WriteStridedColl;
	fd->fns->ADIOI_xxx_SeekIndividual = ADIOI_PFS_SeekIndividual;
	fd->fns->ADIOI_xxx_Fcntl = ADIOI_PFS_Fcntl;
	fd->fns->ADIOI_xxx_SetInfo = ADIOI_PFS_SetInfo;
	fd->fns->ADIOI_xxx_ReadStrided = ADIOI_PFS_ReadStrided;
	fd->fns->ADIOI_xxx_WriteStrided = ADIOI_PFS_WriteStrided;
	fd->fns->ADIOI_xxx_Close = ADIOI_PFS_Close;
	fd->fns->ADIOI_xxx_IreadContig = ADIOI_PFS_IreadContig;
	fd->fns->ADIOI_xxx_IwriteContig = ADIOI_PFS_IwriteContig;
	fd->fns->ADIOI_xxx_ReadDone = ADIOI_PFS_ReadDone;
	fd->fns->ADIOI_xxx_WriteDone = ADIOI_PFS_WriteDone;
	fd->fns->ADIOI_xxx_ReadComplete = ADIOI_PFS_ReadComplete;
	fd->fns->ADIOI_xxx_WriteComplete = ADIOI_PFS_WriteComplete;
	fd->fns->ADIOI_xxx_IreadStrided = ADIOI_PFS_IreadStrided;
	fd->fns->ADIOI_xxx_IwriteStrided = ADIOI_PFS_IwriteStrided;
	fd->fns->ADIOI_xxx_Flush = ADIOI_PFS_Flush;
	fd->fns->ADIOI_xxx_Resize = ADIOI_PFS_Resize;
#else
	printf("ADIOI_SetFunctions: ROMIO has not been configured to use the PFS file system\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
#endif
	break;

    case ADIO_PIOFS:
#ifdef __PIOFS	
	fd->fns->ADIOI_xxx_Open = ADIOI_PIOFS_Open;
	fd->fns->ADIOI_xxx_ReadContig = ADIOI_PIOFS_ReadContig;
	fd->fns->ADIOI_xxx_WriteContig = ADIOI_PIOFS_WriteContig;
	fd->fns->ADIOI_xxx_ReadStridedColl = ADIOI_PIOFS_ReadStridedColl;
	fd->fns->ADIOI_xxx_WriteStridedColl = ADIOI_PIOFS_WriteStridedColl;
	fd->fns->ADIOI_xxx_SeekIndividual = ADIOI_PIOFS_SeekIndividual;
	fd->fns->ADIOI_xxx_Fcntl = ADIOI_PIOFS_Fcntl;
	fd->fns->ADIOI_xxx_SetInfo = ADIOI_PIOFS_SetInfo;
	fd->fns->ADIOI_xxx_ReadStrided = ADIOI_PIOFS_ReadStrided;
	fd->fns->ADIOI_xxx_WriteStrided = ADIOI_PIOFS_WriteStrided;
	fd->fns->ADIOI_xxx_Close = ADIOI_PIOFS_Close;
	fd->fns->ADIOI_xxx_IreadContig = ADIOI_PIOFS_IreadContig;
	fd->fns->ADIOI_xxx_IwriteContig = ADIOI_PIOFS_IwriteContig;
	fd->fns->ADIOI_xxx_ReadDone = ADIOI_PIOFS_ReadDone;
	fd->fns->ADIOI_xxx_WriteDone = ADIOI_PIOFS_WriteDone;
	fd->fns->ADIOI_xxx_ReadComplete = ADIOI_PIOFS_ReadComplete;
	fd->fns->ADIOI_xxx_WriteComplete = ADIOI_PIOFS_WriteComplete;
	fd->fns->ADIOI_xxx_IreadStrided = ADIOI_PIOFS_IreadStrided;
	fd->fns->ADIOI_xxx_IwriteStrided = ADIOI_PIOFS_IwriteStrided;
	fd->fns->ADIOI_xxx_Flush = ADIOI_PIOFS_Flush;
	fd->fns->ADIOI_xxx_Resize = ADIOI_PIOFS_Resize;
#else
	printf("ADIOI_SetFunctions: ROMIO has not been configured to use the PIOFS file system\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
#endif
	break;

    case ADIO_UFS:
#ifdef __UFS	
	fd->fns->ADIOI_xxx_Open = ADIOI_UFS_Open;
	fd->fns->ADIOI_xxx_ReadContig = ADIOI_UFS_ReadContig;
	fd->fns->ADIOI_xxx_WriteContig = ADIOI_UFS_WriteContig;
	fd->fns->ADIOI_xxx_ReadStridedColl = ADIOI_UFS_ReadStridedColl;
	fd->fns->ADIOI_xxx_WriteStridedColl = ADIOI_UFS_WriteStridedColl;
	fd->fns->ADIOI_xxx_SeekIndividual = ADIOI_UFS_SeekIndividual;
	fd->fns->ADIOI_xxx_Fcntl = ADIOI_UFS_Fcntl;
	fd->fns->ADIOI_xxx_SetInfo = ADIOI_UFS_SetInfo;
	fd->fns->ADIOI_xxx_ReadStrided = ADIOI_UFS_ReadStrided;
	fd->fns->ADIOI_xxx_WriteStrided = ADIOI_UFS_WriteStrided;
	fd->fns->ADIOI_xxx_Close = ADIOI_UFS_Close;
	fd->fns->ADIOI_xxx_IreadContig = ADIOI_UFS_IreadContig;
	fd->fns->ADIOI_xxx_IwriteContig = ADIOI_UFS_IwriteContig;
	fd->fns->ADIOI_xxx_ReadDone = ADIOI_UFS_ReadDone;
	fd->fns->ADIOI_xxx_WriteDone = ADIOI_UFS_WriteDone;
	fd->fns->ADIOI_xxx_ReadComplete = ADIOI_UFS_ReadComplete;
	fd->fns->ADIOI_xxx_WriteComplete = ADIOI_UFS_WriteComplete;
	fd->fns->ADIOI_xxx_IreadStrided = ADIOI_UFS_IreadStrided;
	fd->fns->ADIOI_xxx_IwriteStrided = ADIOI_UFS_IwriteStrided;
	fd->fns->ADIOI_xxx_Flush = ADIOI_UFS_Flush;
	fd->fns->ADIOI_xxx_Resize = ADIOI_UFS_Resize;
#else
	printf("ADIOI_SetFunctions: ROMIO has not been configured to use the UFS file system\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
#endif
	break;

    case ADIO_NFS:
#ifdef __NFS	
	fd->fns->ADIOI_xxx_Open = ADIOI_NFS_Open;
	fd->fns->ADIOI_xxx_ReadContig = ADIOI_NFS_ReadContig;
	fd->fns->ADIOI_xxx_WriteContig = ADIOI_NFS_WriteContig;
	fd->fns->ADIOI_xxx_ReadStridedColl = ADIOI_NFS_ReadStridedColl;
	fd->fns->ADIOI_xxx_WriteStridedColl = ADIOI_NFS_WriteStridedColl;
	fd->fns->ADIOI_xxx_SeekIndividual = ADIOI_NFS_SeekIndividual;
	fd->fns->ADIOI_xxx_Fcntl = ADIOI_NFS_Fcntl;
	fd->fns->ADIOI_xxx_SetInfo = ADIOI_NFS_SetInfo;
	fd->fns->ADIOI_xxx_ReadStrided = ADIOI_NFS_ReadStrided;
	fd->fns->ADIOI_xxx_WriteStrided = ADIOI_NFS_WriteStrided;
	fd->fns->ADIOI_xxx_Close = ADIOI_NFS_Close;
	fd->fns->ADIOI_xxx_IreadContig = ADIOI_NFS_IreadContig;
	fd->fns->ADIOI_xxx_IwriteContig = ADIOI_NFS_IwriteContig;
	fd->fns->ADIOI_xxx_ReadDone = ADIOI_NFS_ReadDone;
	fd->fns->ADIOI_xxx_WriteDone = ADIOI_NFS_WriteDone;
	fd->fns->ADIOI_xxx_ReadComplete = ADIOI_NFS_ReadComplete;
	fd->fns->ADIOI_xxx_WriteComplete = ADIOI_NFS_WriteComplete;
	fd->fns->ADIOI_xxx_IreadStrided = ADIOI_NFS_IreadStrided;
	fd->fns->ADIOI_xxx_IwriteStrided = ADIOI_NFS_IwriteStrided;
	fd->fns->ADIOI_xxx_Flush = ADIOI_NFS_Flush;
	fd->fns->ADIOI_xxx_Resize = ADIOI_NFS_Resize;
#else
	printf("ADIOI_SetFunctions: ROMIO has not been configured to use the NFS file system\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
#endif
	break;

    case ADIO_HFS:
#ifdef __HFS	
	fd->fns->ADIOI_xxx_Open = ADIOI_HFS_Open;
	fd->fns->ADIOI_xxx_ReadContig = ADIOI_HFS_ReadContig;
	fd->fns->ADIOI_xxx_WriteContig = ADIOI_HFS_WriteContig;
	fd->fns->ADIOI_xxx_ReadStridedColl = ADIOI_HFS_ReadStridedColl;
	fd->fns->ADIOI_xxx_WriteStridedColl = ADIOI_HFS_WriteStridedColl;
	fd->fns->ADIOI_xxx_SeekIndividual = ADIOI_HFS_SeekIndividual;
	fd->fns->ADIOI_xxx_Fcntl = ADIOI_HFS_Fcntl;
	fd->fns->ADIOI_xxx_SetInfo = ADIOI_HFS_SetInfo;
	fd->fns->ADIOI_xxx_ReadStrided = ADIOI_HFS_ReadStrided;
	fd->fns->ADIOI_xxx_WriteStrided = ADIOI_HFS_WriteStrided;
	fd->fns->ADIOI_xxx_Close = ADIOI_HFS_Close;
	fd->fns->ADIOI_xxx_IreadContig = ADIOI_HFS_IreadContig;
	fd->fns->ADIOI_xxx_IwriteContig = ADIOI_HFS_IwriteContig;
	fd->fns->ADIOI_xxx_ReadDone = ADIOI_HFS_ReadDone;
	fd->fns->ADIOI_xxx_WriteDone = ADIOI_HFS_WriteDone;
	fd->fns->ADIOI_xxx_ReadComplete = ADIOI_HFS_ReadComplete;
	fd->fns->ADIOI_xxx_WriteComplete = ADIOI_HFS_WriteComplete;
	fd->fns->ADIOI_xxx_IreadStrided = ADIOI_HFS_IreadStrided;
	fd->fns->ADIOI_xxx_IwriteStrided = ADIOI_HFS_IwriteStrided;
	fd->fns->ADIOI_xxx_Flush = ADIOI_HFS_Flush;
	fd->fns->ADIOI_xxx_Resize = ADIOI_HFS_Resize;
#else
	printf("ADIOI_SetFunctions: ROMIO has not been configured to use the HFS file system\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
#endif
	break;

    case ADIO_XFS:
#ifdef __XFS	
	fd->fns->ADIOI_xxx_Open = ADIOI_XFS_Open;
	fd->fns->ADIOI_xxx_ReadContig = ADIOI_XFS_ReadContig;
	fd->fns->ADIOI_xxx_WriteContig = ADIOI_XFS_WriteContig;
	fd->fns->ADIOI_xxx_ReadStridedColl = ADIOI_XFS_ReadStridedColl;
	fd->fns->ADIOI_xxx_WriteStridedColl = ADIOI_XFS_WriteStridedColl;
	fd->fns->ADIOI_xxx_SeekIndividual = ADIOI_XFS_SeekIndividual;
	fd->fns->ADIOI_xxx_Fcntl = ADIOI_XFS_Fcntl;
	fd->fns->ADIOI_xxx_SetInfo = ADIOI_XFS_SetInfo;
	fd->fns->ADIOI_xxx_ReadStrided = ADIOI_XFS_ReadStrided;
	fd->fns->ADIOI_xxx_WriteStrided = ADIOI_XFS_WriteStrided;
	fd->fns->ADIOI_xxx_Close = ADIOI_XFS_Close;
	fd->fns->ADIOI_xxx_IreadContig = ADIOI_XFS_IreadContig;
	fd->fns->ADIOI_xxx_IwriteContig = ADIOI_XFS_IwriteContig;
	fd->fns->ADIOI_xxx_ReadDone = ADIOI_XFS_ReadDone;
	fd->fns->ADIOI_xxx_WriteDone = ADIOI_XFS_WriteDone;
	fd->fns->ADIOI_xxx_ReadComplete = ADIOI_XFS_ReadComplete;
	fd->fns->ADIOI_xxx_WriteComplete = ADIOI_XFS_WriteComplete;
	fd->fns->ADIOI_xxx_IreadStrided = ADIOI_XFS_IreadStrided;
	fd->fns->ADIOI_xxx_IwriteStrided = ADIOI_XFS_IwriteStrided;
	fd->fns->ADIOI_xxx_Flush = ADIOI_XFS_Flush;
	fd->fns->ADIOI_xxx_Resize = ADIOI_XFS_Resize;
#else
	printf("ADIOI_SetFunctions: ROMIO has not been configured to use the XFS file system\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
#endif
	break;

    case ADIO_SFS:
#ifdef __SFS	
	fd->fns->ADIOI_xxx_Open = ADIOI_SFS_Open;
	fd->fns->ADIOI_xxx_ReadContig = ADIOI_SFS_ReadContig;
	fd->fns->ADIOI_xxx_WriteContig = ADIOI_SFS_WriteContig;
	fd->fns->ADIOI_xxx_ReadStridedColl = ADIOI_SFS_ReadStridedColl;
	fd->fns->ADIOI_xxx_WriteStridedColl = ADIOI_SFS_WriteStridedColl;
	fd->fns->ADIOI_xxx_SeekIndividual = ADIOI_SFS_SeekIndividual;
	fd->fns->ADIOI_xxx_Fcntl = ADIOI_SFS_Fcntl;
	fd->fns->ADIOI_xxx_SetInfo = ADIOI_SFS_SetInfo;
	fd->fns->ADIOI_xxx_ReadStrided = ADIOI_SFS_ReadStrided;
	fd->fns->ADIOI_xxx_WriteStrided = ADIOI_SFS_WriteStrided;
	fd->fns->ADIOI_xxx_Close = ADIOI_SFS_Close;
	fd->fns->ADIOI_xxx_IreadContig = ADIOI_SFS_IreadContig;
	fd->fns->ADIOI_xxx_IwriteContig = ADIOI_SFS_IwriteContig;
	fd->fns->ADIOI_xxx_ReadDone = ADIOI_SFS_ReadDone;
	fd->fns->ADIOI_xxx_WriteDone = ADIOI_SFS_WriteDone;
	fd->fns->ADIOI_xxx_ReadComplete = ADIOI_SFS_ReadComplete;
	fd->fns->ADIOI_xxx_WriteComplete = ADIOI_SFS_WriteComplete;
	fd->fns->ADIOI_xxx_IreadStrided = ADIOI_SFS_IreadStrided;
	fd->fns->ADIOI_xxx_IwriteStrided = ADIOI_SFS_IwriteStrided;
	fd->fns->ADIOI_xxx_Flush = ADIOI_SFS_Flush;
	fd->fns->ADIOI_xxx_Resize = ADIOI_SFS_Resize;
#else
	printf("ADIOI_SetFunctions: ROMIO has not been configured to use the SFS file system\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
#endif
	break;

    case ADIO_PVFS:
#ifdef __PVFS	
	fd->fns->ADIOI_xxx_Open = ADIOI_PVFS_Open;
	fd->fns->ADIOI_xxx_ReadContig = ADIOI_PVFS_ReadContig;
	fd->fns->ADIOI_xxx_WriteContig = ADIOI_PVFS_WriteContig;
	fd->fns->ADIOI_xxx_ReadStridedColl = ADIOI_PVFS_ReadStridedColl;
	fd->fns->ADIOI_xxx_WriteStridedColl = ADIOI_PVFS_WriteStridedColl;
	fd->fns->ADIOI_xxx_SeekIndividual = ADIOI_PVFS_SeekIndividual;
	fd->fns->ADIOI_xxx_Fcntl = ADIOI_PVFS_Fcntl;
	fd->fns->ADIOI_xxx_SetInfo = ADIOI_PVFS_SetInfo;
	fd->fns->ADIOI_xxx_ReadStrided = ADIOI_PVFS_ReadStrided;
	fd->fns->ADIOI_xxx_WriteStrided = ADIOI_PVFS_WriteStrided;
	fd->fns->ADIOI_xxx_Close = ADIOI_PVFS_Close;
	fd->fns->ADIOI_xxx_IreadContig = ADIOI_PVFS_IreadContig;
	fd->fns->ADIOI_xxx_IwriteContig = ADIOI_PVFS_IwriteContig;
	fd->fns->ADIOI_xxx_ReadDone = ADIOI_PVFS_ReadDone;
	fd->fns->ADIOI_xxx_WriteDone = ADIOI_PVFS_WriteDone;
	fd->fns->ADIOI_xxx_ReadComplete = ADIOI_PVFS_ReadComplete;
	fd->fns->ADIOI_xxx_WriteComplete = ADIOI_PVFS_WriteComplete;
	fd->fns->ADIOI_xxx_IreadStrided = ADIOI_PVFS_IreadStrided;
	fd->fns->ADIOI_xxx_IwriteStrided = ADIOI_PVFS_IwriteStrided;
	fd->fns->ADIOI_xxx_Flush = ADIOI_PVFS_Flush;
	fd->fns->ADIOI_xxx_Resize = ADIOI_PVFS_Resize;
#else
	printf("ADIOI_SetFunctions: ROMIO has not been configured to use the PVFS file system\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
#endif
	break;

    default:
	printf("ADIOI_SetFunctions: Unsupported file system type\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
	break;
    }
}
