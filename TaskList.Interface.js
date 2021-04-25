var TaskList_Cmd_Delete = cwrap('TaskList_Cmd_Delete', null, ['string', 'number', 'number']);
var TaskList_Cmd_Insert = cwrap('TaskList_Cmd_Insert', null, ['string', 'string']);
var TaskList_Cmd_GetIndex = cwrap('TaskList_Cmd_GetIndex', null, ['string']);
var TaskList_Cmd_GetItem = cwrap('TaskList_Cmd_GetItem', null, ['string', 'number', 'number']);
var TaskList_Cmd_Update = cwrap('TaskList_Cmd_Update', null, ['string', 'number', 'string']);

//--------------------------------------------------------------------------------
function TaskList_DeleteSelected(name) {

   var items = document.getElementsByName(name+"_checkbox");
   var count = 0;

   for (i = 0; i < items.length; i++) {
      if (items[i].checked) count++;
   }

   var batch = new Int32Array(count);
   ofs = name.length + 5;

   for (i = j = 0; i < items.length; i++) {
      if (items[i].checked) {
         id = items[i].getAttribute("id").slice(ofs);
         batch[j++] = id;
      }
   }

   var heap = Module._malloc(count*4);
   Module.HEAP32.set(batch, heap / 4);

   TaskList_Cmd_Delete(name, count, heap, null);

   Module._free(heap);
}
//--------------------------------------------------------------------------------
function TaskList_AddIndexItem(name, item, item_id) {
   var table = document.getElementById(name+"_table");
   var row = table.insertRow(-1);
   var cell1 = row.insertCell(0);
   var cell2 = row.insertCell(1);
   var cell3 = row.insertCell(2);

   cell1.style = "width: 32px; text-align: right; vertical-align: top;";
   cell1.innerHTML = "<input name='"+name+"_checkbox' id='"+name+"_chk_"+item_id+"' type='checkbox'>";

   cell2.innerHTML = item;

   cell3.style = "text-align: right; vertical-align: top; ";
   cell3.innerHTML = "<a onclick='TaskList_GetItemForEdit(\""+name+"\", "+item_id+")' style='cursor: pointer; ' >&#x1F4DD;</a>";
}
//--------------------------------------------------------------------------------
function TaskList_UpdateIndexItem(name, item, item_id) {
   var row_of_item = document.getElementById(name+"_chk_"+item_id).parentElement.parentElement;
   row_of_item.cells[1].innerHTML = item;
}
//--------------------------------------------------------------------------------
function TaskList_DeleteItem(name, item_id) {
   var chk = document.getElementById(name+"_chk_"+item_id);
   var i = chk.parentNode.parentNode.rowIndex;
   document.getElementById(name+"_table").deleteRow(i);
}
//--------------------------------------------------------------------------------
var TaskList_GetItemForEdit_Callback = null; 

function TaskList_GetItemForEdit(name, item_id) {

   if (TaskList_GetItemForEdit_Callback == null) {
      TaskList_GetItemForEdit_Callback = addFunction(
         function(cmd, param, name, data) {
            document.getElementById("TaskList_"+UTF8ToString(name)+"_edit_txt").value = UTF8ToString(data);
            TaskList_OpenEditDlg(UTF8ToString(name), param);
         }, 'viiii'
      );
   }

   TaskList_Cmd_GetItem(name, item_id, TaskList_GetItemForEdit_Callback);
}
//--------------------------------------------------------------------------------
function TaskList_OpenEditDlg(name, item_id) {
   document.getElementById("TaskList_"+name+"_edit_itemid").value = item_id;
   document.getElementById("TaskList_"+name+"_edit_dlg_bkg").style.display = "initial";
   document.getElementById("TaskList_"+name+"_edit_dlg").style.display = "initial";
}
//--------------------------------------------------------------------------------
function TaskList_CloseEditDlg(name) {
   document.getElementById("TaskList_"+name+"_edit_dlg").style.display = "none";
   document.getElementById("TaskList_"+name+"_edit_dlg_bkg").style.display = "none";
   document.getElementById("TaskList_"+name+"_edit_txt").value = "";
}
//--------------------------------------------------------------------------------
function TaskList_SubmitItem(name) {
   var item_id = Number(document.getElementById("TaskList_"+name+"_edit_itemid").value);
   var txt = document.getElementById("TaskList_"+name+"_edit_txt").value;

   if (isNaN(item_id)) {
      TaskList_Cmd_Insert(name, txt);
   } else {
      TaskList_Cmd_Update(name, item_id, txt, null);
   }
  
   TaskList_CloseEditDlg(name);
}
//--------------------------------------------------------------------------------
function TaskList_DeployApplet(obj) {
   var name = obj.id;
   var applet_template = "" +
   "<div style='background: #000080; padding: 8px; color: #d0a518; font-weight: bold; font-family: Arial, Helvetica, sans-serif; '>" +
      "<table style='color: #d0a518; font-weight: bold; width: 100%; '><tr>"+
         "<td width='100%' >"+obj.getAttribute("data-title")+"</td>"+
         "<td style='cursor: pointer; ' onclick='TaskList_OpenEditDlg(\""+name+"\", \"new_item\")'>&#x1F4C4;</td>"+
         "<td style='cursor: pointer; ' onclick='TaskList_DeleteSelected(\""+name+"\")'>&#x1f5d1;&#xfe0f;</td>"+
      "</tr></table>"+
   "</div>"+
   "<div style='border-style: solid; border-width: 1px;' >"+
      "<table id='"+name+"_table' width='100%'>"+
      "</table>"+
   "</div>"+

   "<div id='TaskList_"+name+"_edit_dlg_bkg' style='display: none; z-index: 1; position: fixed; left: 0; top: 0; width: 100%; height: 100%; overflow: auto; background-color: rgb(0,0,0,0.2); ' >"+
      "<input type='hidden' id='TaskList_"+name+"_edit_itemid' value='???'>"+
      "<div id='TaskList_"+name+"_edit_dlg' style='display: none; position: fixed; top: 50%; left: 50%; width: 320px; margin-left: -160px; ' >"+
         "<table style='background: #000080; text-align: center; border-top-left-radius: 8px; border-top-right-radius: 8px; font-weight: bold; color: #d0a518; padding: 4px; width: 100%; width: 100%; font-family: Arial, Helvetica, sans-serif; '><tr>"+
            "<td>"+obj.getAttribute("data-title")+": Edit</td>"+
         "</tr></table>"+
         "<table style='background: #D0D0D0; width: 100%; border: 1px solid black; border-collapse: collapse; padding: 8px; '>"+
            "<tr><td><textarea maxlength='128' id='TaskList_"+name+"_edit_txt' style='width: 100%; height: 100px; resize: none; '></textarea> </td></tr>"+
            "<tr><td style='text-align: right;' ><button type='button' onclick='TaskList_CloseEditDlg(\""+name+"\")'>Cancel</button><button type='button' onclick='TaskList_SubmitItem(\""+name+"\")'>Save</button></td></tr>"+
         "</table>"+
      "</div>"+
   "</div>";

   obj.innerHTML = applet_template;
   TaskList_Cmd_GetIndex(name);
}
//--------------------------------------------------------------------------------

