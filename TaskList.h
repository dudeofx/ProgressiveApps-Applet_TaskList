#ifndef __TaskList_H__
#define __TaskList_H__

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <emscripten.h>
#include <emscripten/fetch.h>

enum {
   _NOP,         // do nothing
   _GET_INDEX,   // retrieve all items; returns INDEX
   _GET_ITEM,    // retrieve 1 item (param == id); returns ITEM
   _INSERT,      // insert new item (id is created by server and returned in param); returns NEW_ITEM
   _DELETE,      // deletes item; returns DELETE
   _UPDATE,      // updates item; returns UPDATE
   _INDEX,       // samples of all items 
   _ITEM,        // a single item
   _NEW_ITEM,    // a newly added item
   _OK,          // command successful
   _ERROR,       // error (param = code)
   _CMD_COUNT
};

typedef struct _tFetchWorkOrder tFetchWorkOrder;
typedef struct _tHeader tHeader;

struct _tFetchWorkOrder {
   void (*JSHandler)(int, int, char *, char *);
   int                  id;
   tHeader             *data;
   emscripten_fetch_t  *fetch_struct;
   tFetchWorkOrder     *next;
};

struct _tHeader {
   char magic1[4];
   int  cmd;
   int  param;
   int  name_len;
   int  body_len;
   char magic2[4];
};

void SubmitWorkOrder(int cmd, int param, char *name, void *body, int body_len, void (*JSHandler)(int, int, char *, char *));
void RemoveWorkOrder(tFetchWorkOrder *wk);

void FetchTransferHandler(emscripten_fetch_t *fetch);
void FetchErrorHandler(emscripten_fetch_t *fetch);

#endif

