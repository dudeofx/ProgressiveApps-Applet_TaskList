#include "TaskList.h"

tFetchWorkOrder *ticket_wheel =  NULL;
int              ticket_uid = 0x1000;

//-----------------------------------------------------------------------
void SubmitWorkOrder(int cmd, int param, char *name, void *body, int body_len, void (*JSHandler)(int, int, char *, char *)) {
   int name_len;
   int data_size;
   char *dst;

   if (cmd <= _NOP) return;
   if (cmd >= _CMD_COUNT) return;

   name_len = strlen(name);

   tFetchWorkOrder *order = malloc(sizeof(tFetchWorkOrder));

   data_size = sizeof(tHeader)+name_len+body_len;

   order->JSHandler = JSHandler;
   order->id = ticket_uid++;
   order->data = malloc(data_size);
   order->fetch_struct = NULL;
   order->next = NULL;

   memcpy(order->data->magic1, "CpH1", 4);
   order->data->cmd = cmd;
   order->data->param = param;
   order->data->name_len = name_len;
   order->data->body_len = body_len;
   memcpy(order->data->magic2, "z2C3", 4);

   dst = ((char *) (order->data)) + sizeof(tHeader);
   memcpy(dst, name, name_len);
   dst += name_len;
   memcpy(dst, body, body_len);

   order->next = ticket_wheel;
   ticket_wheel = order;
    
   emscripten_fetch_attr_t attr;
   emscripten_fetch_attr_init(&attr);

   attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
   strcpy(attr.requestMethod, "CUSTOM");
   attr.requestData = (char *) order->data;
   attr.requestDataSize = data_size;

   attr.onsuccess = FetchTransferHandler;
   attr.onerror = FetchErrorHandler;
   attr.userData = order;

   order->fetch_struct = emscripten_fetch(&attr, "TaskList.php");
}
//-----------------------------------------------------------------------
void RemoveWorkOrder(tFetchWorkOrder *wk) {
   if (ticket_wheel == wk) {
      ticket_wheel = wk->next;
      free(wk);
   } else {
      tFetchWorkOrder *tmp = ticket_wheel;
      while (tmp->next != wk) tmp = tmp->next;
      tmp->next = wk->next;
      free(wk);
   }
}
//-----------------------------------------------------------------------
void FetchTransferHandler(emscripten_fetch_t *fetch) {
   tFetchWorkOrder *wk = (tFetchWorkOrder *) fetch->userData;
   tHeader *header = (tHeader *) (fetch->data);

   switch (header->cmd) {

      case _INDEX: {
         int count = header->param;
         int *index = (int *) ( ((char *) (fetch->data)) + sizeof(tHeader) + header->name_len);
         char *buff = ((char *) index) + count*sizeof(int)*3;
         char *name_ptr = ((char *) (fetch->data)) + sizeof(tHeader);

         char name[64];
         sprintf(name, "%.*s", header->name_len, name_ptr);
         char item[256];

         for (int i = 0; i < count; i++) {
            int item_id = index[i*3];
            int offset = index[i*3+2];
            int size = index[i*3+1];

            sprintf(item, "%.*s", size, buff+offset);
            EM_ASM({
               name = UTF8ToString($0);
               item = UTF8ToString($1);
               TaskList_AddIndexItem(name, item, $2);
            }, name, item, item_id);
         }
      } break;
      case _DELETE: {
         int count = header->param;
         int *index = (int *) ( ((char *) (fetch->data)) + sizeof(tHeader) + header->name_len);

         char *name_ptr = ((char *) (fetch->data)) + sizeof(tHeader);
         char name[64];
         sprintf(name, "%.*s", header->name_len, name_ptr);

         for (int i = 0; i < count; i++) {
            EM_ASM({
               name = UTF8ToString($0);
               TaskList_DeleteItem(name, $1);
            }, name, index[i]);
         }

      } break;
      case _NEW_ITEM: {
         int item_id = header->param;

         char *name_ptr = ((char *) (fetch->data)) + sizeof(tHeader);
         char name[64];
         sprintf(name, "%.*s", header->name_len, name_ptr);

         char *item_ptr = ( ((char *) (fetch->data)) + sizeof(tHeader) + header->name_len);
         char item[256];
         sprintf(item, "%.*s", header->body_len, item_ptr);

         EM_ASM({
            name = UTF8ToString($0);
            item = UTF8ToString($1);
            TaskList_AddIndexItem(name, item, $2);
         }, name, item, item_id);
      } break;
      case _OK:
//         wk->JSHandler(header->cmd, header->param, "", "");
         break;
      case _ITEM: {
         char *name_ptr = ((char *) (fetch->data)) + sizeof(tHeader);
         char name[64];
         sprintf(name, "%.*s", header->name_len, name_ptr);

         char *item_ptr = ( ((char *) (fetch->data)) + sizeof(tHeader) + header->name_len);
         char item[256];
         sprintf(item, "%.*s", header->body_len, item_ptr);

         wk->JSHandler(header->cmd, header->param, name, item);
      } break;
      case _UPDATE: {
         int item_id = header->param;

         char *name_ptr = ((char *) (fetch->data)) + sizeof(tHeader);
         char name[64];
         sprintf(name, "%.*s", header->name_len, name_ptr);

         char *item_ptr = ( ((char *) (fetch->data)) + sizeof(tHeader) + header->name_len);
         char item[256];
         sprintf(item, "%.*s", header->body_len, item_ptr);

         EM_ASM({
            name = UTF8ToString($0);
            item = UTF8ToString($1);
            TaskList_UpdateIndexItem(name, item, $2);
         }, name, item, item_id);
      } break;
      case _ERROR:
         break;
      default:
         break;
   }

   emscripten_fetch_close(fetch);
   RemoveWorkOrder(wk);
}
//-----------------------------------------------------------------------
void FetchErrorHandler(emscripten_fetch_t *fetch) {
   tFetchWorkOrder *wk = (tFetchWorkOrder *) fetch->userData;
   printf("error\n");
//         wk->JSHandler(???);
   emscripten_fetch_close(fetch);
   RemoveWorkOrder(wk);
}
//-----------------------------------------------------------------------
EMSCRIPTEN_KEEPALIVE 
void TaskList_Cmd_Insert(char *name, char *data) {
   SubmitWorkOrder(_INSERT, 0, name, data, strlen(data), NULL);
}
//-----------------------------------------------------------------------
EMSCRIPTEN_KEEPALIVE
void TaskList_Cmd_GetIndex(char *name) {
   SubmitWorkOrder(_GET_INDEX, 0, name, "", 0, NULL);
}
//-----------------------------------------------------------------------
EMSCRIPTEN_KEEPALIVE
void TaskList_Cmd_Delete(char *name, int count, int *batch) {
   SubmitWorkOrder(_DELETE, count, name, batch, count*4, NULL);
}
//-----------------------------------------------------------------------
EMSCRIPTEN_KEEPALIVE
void TaskList_Cmd_GetItem(char *name, int item_id, void (*JSHandler)(int, int, char *, char *) ) {
   SubmitWorkOrder(_GET_ITEM, item_id, name, NULL, 0, JSHandler);
}
//-----------------------------------------------------------------------
EMSCRIPTEN_KEEPALIVE
void TaskList_Cmd_Update(char *name, int item_id, char *update) {
   SubmitWorkOrder(_UPDATE, item_id, name, update, strlen(update), NULL);
}
//-----------------------------------------------------------------------
int main() {
   EM_ASM(
      var x = document.getElementsByClassName("appTaskList");
      for (i = 0; i < x.length; i++) TaskList_DeployApplet(x[i]);
   );

   emscripten_exit_with_live_runtime();
}
//-----------------------------------------------------------------------

