#pragma once
#include <iostream>
#include <fstream>
#include <unordered_map>
#include "DiskController.h"
#include "Page.h"

class BufferManager {
public:
    DiskController * diskController;

    vector<Page> bufferPool;
    int sizeBufferPool;
    int sizePage;
    
    vector< vector<int> > pageTableLRU; //pageTableLRU
    vector< vector<int> > pageTableMRU; //pageTableMRU
    vector< vector<int> > pageTableCLOCK; //pageTableMRU

    BufferManager(DiskController * diskController){
        this->sizeBufferPool = 4;
        this->diskController = diskController;
        this->sizePage = this->diskController->sizeBloque;
        bufferPool.resize(this->sizeBufferPool);
        for (auto& frame : bufferPool){
            frame = Page(this->sizePage);
        }

        pageTableLRU.resize(sizeBufferPool);
        for(auto& frame : pageTableLRU){
            frame.resize(5); // frameId,Page,id,DirtyBit,PinCount,LastUsed
        }
        
        for (int i=0; i<pageTableLRU.size(); i++){
            for (int j=0; j<pageTableLRU[0].size(); j++){
                if(j==0){
                    pageTableLRU[i][j] = i;
                } else {
                    pageTableLRU[i][j] = -1;
                }
            }
        }
        pageTableMRU.resize(sizeBufferPool);
        for(auto& frame : pageTableMRU){
            frame.resize(5); // frameId,Page,id,DirtyBit,PinCount,MostUsed
        }
        for (int i=0; i<pageTableMRU.size(); i++){
            for (int j=0; j<pageTableMRU[0].size(); j++){
                if(j==0){
                    pageTableMRU[i][j] = i;
                } else {
                    pageTableMRU[i][j] = -1;
                }
            }
        }

        pageTableCLOCK.resize(sizeBufferPool);
        for(auto& frame : pageTableCLOCK){
            frame.resize(6); // frameId,Pageid,DirtyBit,BitRef,puntero,PinCount
        }
        for (int i=0; i<pageTableCLOCK.size(); i++){
            for (int j=0; j<pageTableCLOCK[0].size(); j++){
                if(j==0){
                    pageTableCLOCK[i][j] = i;
                } else {
                    pageTableCLOCK[i][j] = -1;
                }
            }
        }
        pageTableCLOCK[0][4] = 1; //Seteo del puntero del reloj


    }
    ~BufferManager() { }


    auto getPageOfBuuferPool(int pageId){
        Page* pagina = CLOCK(pageId);
        return pagina;
    }
    
    //Funcion clocl
    Page* CLOCK(int pageId){
        //CASO1: el frame se encuentra en el buffer pool 
        for (int i=0; i<(int)pageTableCLOCK.size(); i++){
            if(pageTableCLOCK[i][1]==pageId){
                if (pageTableCLOCK[i][3] == 0) {//BitRef
                    pageTableCLOCK[i][3] = 1;
                }
                pageTableCLOCK[i][5]++;//PinCount
                return &bufferPool[i];
            }
        }
        //CASO2: el frame no se encuentra pero hay espacio libre en el buffer pool
        for (int i=0; i<pageTableCLOCK.size(); i++){
            if(pageTableCLOCK[i][1]==-1){
            cout<<"Entra"<<endl;
                //Seteo de pagina en el frame pageId,DirtyBit,RefBit,PinCount
                pageTableCLOCK[i][1] = pageId;
                pageTableCLOCK[i][2] = 0;
                pageTableCLOCK[i][3] = 1;
                pageTableCLOCK[i][5] = 1;
                    ifstream file;
                    file.open("disk/bloque" + to_string(pageId) + ".bin", ios::in | ios::binary);
                    file.read(bufferPool[i].data, this->diskController->sizeBloque);
                    file.close();
                return &bufferPool[i];
            }  
        }
        //CASO3: No se encuentra el frame y esta lleno el buffer pool
        //El puntero del reloj empieza a moverse y buscar el frame a reemplazar
        
        while (true) {
            //showpageTableCLOCK();
            int posPtr = posicionPuntero();

            if(pageTableCLOCK[posPtr][3] == 1){
                pageTableCLOCK[posPtr][3] = 0;
                moverPuntero();
            }
            else {
                if (pageTableCLOCK[posPtr][5] != 0){
                    showpageTableCLOCK();
                    char c;
                    cout<<" Este bloque fue solicitado "<<pageTableCLOCK[posPtr][5]<<" veces. Desea liberar el bloque "<<pageTableCLOCK[posPtr][1]<<" (y/n): ";
                    cin>>c;
                    if (c == 'n') {
                        moverPuntero();
                        continue;
                    }
                }
                if (pageTableCLOCK[posPtr][2] == 0) {
                    pageTableCLOCK[posPtr][1] = pageId;
                    pageTableCLOCK[posPtr][2] = 0;
                    pageTableCLOCK[posPtr][3] = 1;
                    pageTableCLOCK[posPtr][5] = 1;
                    moverPuntero();
                    ifstream file;
                    file.open("disk/bloque" + to_string(pageId) + ".bin", ios::in | ios::binary);
                    file.read(bufferPool[posPtr].data, this->diskController->sizeBloque);
                    file.close();
                    return &bufferPool[posPtr];
                }
                else {
                    return UnpinFrame(pageId);
                }
            }
        }
        return nullptr;
    }
    int posicionPuntero(){
        for(int i=0; i<(int)pageTableCLOCK.size(); i++){
            if(pageTableCLOCK[i][4] == 1){
                return i;
            }
        }
        return 0;
    }
    void moverPuntero(){
        for(int i=0; i<(int)pageTableCLOCK.size(); i++){
            if(pageTableCLOCK[i][4] == 1){
                pageTableCLOCK[i][4] = -1;
                if(i==pageTableCLOCK.size()-1){
                    pageTableCLOCK[0][4] = 1;
                }
                else {
                    pageTableCLOCK[i+1][4] = 1;
                }
                break;
            }
        }
    }

    Page* LRU(int pageId){
        for (int i=0; i<pageTableLRU.size(); i++){
            if(pageTableLRU[i][1]==pageId){
                pageTableLRU[i][3]++;
                for(int k=0; k<pageTableLRU.size(); k++){
                    if(pageTableLRU[k][1]!=-1){
                        pageTableLRU[k][4]++;
                    }
                }
                pageTableLRU[i][4] = 0;
                return &bufferPool[i];
            }
        }
        for (int i=0; i<pageTableLRU.size(); i++){
            if(pageTableLRU[i][1]==-1){
                for(int k=0; k<pageTableLRU.size(); k++){
                    if(pageTableLRU[k][1]!=-1){
                        pageTableLRU[k][4]++;
                    }
                }
                pageTableLRU[i][1] = pageId;
                pageTableLRU[i][2] = 0;
                pageTableLRU[i][3] = 0;
                pageTableLRU[i][4] = 0;
                
                return &bufferPool[i];
            }  
        }

        int mayor = pageTableLRU[0][4];
        int pos=0;
        for (int i=0; i<pageTableLRU.size(); i++){
            if(pageTableLRU[i][4]>mayor){
                if (pageTableLRU[i][2] == 0) {
                    mayor = pageTableLRU[i][4];
                    pos = i;
                }
                else{
                    char confirmacion;
                    cout<<" Se realizaron "<<pageTableLRU[i][3]<<" cambios en esta pagina, desea guardar los cambios(y/n): ";
                    cin>>confirmacion;
                    if (confirmacion == 'y') {
                        ofstream save;
                        save.open("disk/bloque"+to_string(pageTableLRU[i][1])+".bin",ios::out | ios::binary);

                        save.write(bufferPool[i].data,diskController->sizeBloque);
                        save.close();
                        diskController->bloqueASector(pageTableLRU[i][1]);
                        cout<<"Se escribio la informacion del bloque "<<i<<" correctamente en el disco."<<endl;
                        mayor = pageTableLRU[i][4];
                        pos = i;
                    }
                    else {
                        mayor = pageTableLRU[i][4];
                        pos = i;
                    }
                }
            }
        }

        if(pos!=-1){
            for(int k=0; k<pageTableLRU.size(); k++){
                if(pageTableLRU[k][1]!=-1){
                    pageTableLRU[k][4]++;
                }
            }
            pageTableLRU[pos][1] = pageId;
            pageTableLRU[pos][2] = 0;
            pageTableLRU[pos][3] = 0;
            pageTableLRU[pos][4] = 0;
            return &bufferPool[pos];
        }

       return nullptr;
    }

    Page* MRU(int pageId){
        for (int i=0; i<pageTableMRU.size(); i++){
            if(pageTableMRU[i][1]==pageId){
                pageTableMRU[i][3]++;
                for(int k=0; k<pageTableMRU.size(); k++){
                    if(pageTableMRU[k][1]!=-1){
                        pageTableMRU[k][4]++;
                    }
                }
                pageTableMRU[i][4] = 0;
                return &bufferPool[i];
            }
        }
        for (int i=0; i<pageTableMRU.size(); i++){
            if(pageTableMRU[i][1]==-1){
                for(int k=0; k<pageTableMRU.size(); k++){
                    if(pageTableMRU[k][1]!=-1){
                        pageTableMRU[k][4]++;
                    }
                }
                pageTableMRU[i][1] = pageId;
                pageTableMRU[i][2] = 0;
                pageTableMRU[i][3] = 0;
                pageTableMRU[i][4] = 0;
                
                return &bufferPool[i];
            }  
        }

        int menor = pageTableMRU[0][4];
        int pos=0;
        for (int i=0; i<pageTableMRU.size(); i++){
            if(pageTableMRU[i][2]==0 && pageTableMRU[i][4]<menor){
                menor = pageTableMRU[i][4];
                pos = i;
            }
        }

        if(pos!=-1){
            for(int k=0; k<pageTableMRU.size(); k++){
                if(pageTableMRU[k][1]!=-1){
                    pageTableMRU[k][4]++;
                }
            }
            pageTableMRU[pos][1] = pageId;
            pageTableMRU[pos][2] = 0;
            pageTableMRU[pos][3] = 0;
            pageTableMRU[pos][4] = 0;
            return &bufferPool[pos];
        }

       return nullptr;
    }

    void PinFrame(int idPage){
        for(int i=0; i<pageTableCLOCK.size(); i++){
            if(pageTableCLOCK[i][1]==idPage){
                pageTableCLOCK[i][2] = 1;
                return;
            }
        }
    }

    Page * UnpinFrame(int pageId){
        int posPtr = posicionPuntero();
        char confirmacion;
        cout<<" Se realizaron cambios en esta pagina, desea guardar los cambios(y/n): ";
        cin>>confirmacion;
        if (confirmacion == 'y') {
            cout<<"Se escribio la informacion del bloque "<<pageTableCLOCK[posPtr][1]<<" correctamente en el disco."<<endl;

                        int a = 0;
                        int pru;
                        cout<<"AQUi\n"<<bufferPool[pageTableCLOCK[posPtr][0]].data<<endl;
                        for(int as = 0; as<5;as++){
                          pru = *reinterpret_cast<int *>(bufferPool[pageTableCLOCK[posPtr][0]].data+a);
                          cout<<pru<<" - ";
                          a += 4;
                          pru = *reinterpret_cast<int *>(bufferPool[pageTableCLOCK[posPtr][0]].data+a);
                          cout<<pru<<" - ";
                          a += 4;
                          pru = *reinterpret_cast<int *>(bufferPool[pageTableCLOCK[posPtr][0]].data+a);
                          cout<<pru<<"\n";
                          a += 4;
                        }
            ofstream save;
            save.open("disk/bloque" + to_string(pageTableCLOCK[posPtr][1]) + ".bin", ios::out | ios::binary);
            save.write(bufferPool[pageTableCLOCK[posPtr][0]].data, diskController->sizeBloque);
            save.close();
            diskController->bloqueASector(pageTableCLOCK[posPtr][1]);


            pageTableCLOCK[posPtr][1] = pageId;
            pageTableCLOCK[posPtr][2] = 0;
            pageTableCLOCK[posPtr][3] = 1;
            pageTableCLOCK[posPtr][5] = 1;
            moverPuntero();
                    ifstream file;
                    file.open("disk/bloque" + to_string(pageId) + ".bin", ios::in | ios::binary);
                    file.read(bufferPool[posPtr].data, this->diskController->sizeBloque);
                    file.close();
            return &bufferPool[posPtr];
        }
        else {
            pageTableCLOCK[posPtr][1] = pageId;
            pageTableCLOCK[posPtr][2] = 0;
            pageTableCLOCK[posPtr][3] = 1;
            pageTableCLOCK[posPtr][5] = 1;
            moverPuntero();
                    ifstream file;
                    file.open("disk/bloque" + to_string(pageId) + ".bin", ios::in | ios::binary);
                    file.read(bufferPool[posPtr].data, this->diskController->sizeBloque);
                    file.close();
            return &bufferPool[posPtr];
        }
    }

    void LimpiarBufferPool(){
        for (int i=0; i<(int)pageTableCLOCK.size(); i++){
            if (pageTableCLOCK[i][2] != 1) {//Dirtybit
                pageTableCLOCK[i][1] = -1;
                pageTableCLOCK[i][2] = -1;
                pageTableCLOCK[i][3] = -1;
                pageTableCLOCK[i][4] = -1;
                pageTableCLOCK[i][5] = -1;
            }
            else {
                char confirmacion;
                cout<<" Se realizaron cambios en esta pagina, desea guardar los cambios(y/n): ";
                cin>>confirmacion;
                if (confirmacion == 'y') {
                    cout<<"Se escribio la informacion del bloque "<<pageTableCLOCK[i][1]<<" correctamente en el disco."<<endl;
                        ofstream save;
                        save.open("disk/bloque" + to_string(pageTableCLOCK[i][0]) + ".bin", ios::out | ios::binary);
                        save.write(bufferPool[pageTableCLOCK[i][0]].data, diskController->sizeBloque);
                        save.close();
                    diskController->bloqueASector(pageTableCLOCK[i][1]);
                    pageTableCLOCK[i][1] = -1;
                    pageTableCLOCK[i][2] = -1;
                    pageTableCLOCK[i][3] = -1;
                    pageTableCLOCK[i][5] = -1;
                    pageTableCLOCK[i][4] = -1;
                }
                else {
                    pageTableCLOCK[i][1] = -1;
                    pageTableCLOCK[i][2] = -1;
                    pageTableCLOCK[i][3] = -1;
                    pageTableCLOCK[i][4] = -1;
                    pageTableCLOCK[i][5] = -1;
                }
            }
        }
        pageTableCLOCK[0][4] = 1; //Seteo del puntero del reloj
    }
    void LimpiarPageBufferPool(int pageId){
        for (int i=0; i<(int)pageTableCLOCK.size(); i++){
            if(pageTableCLOCK[i][1]==pageId){
                if (pageTableCLOCK[i][2] != 1) {//Dirtybit
                    pageTableCLOCK[i][1] = -1;
                    pageTableCLOCK[i][2] = -1;
                    pageTableCLOCK[i][3] = -1;
                    pageTableCLOCK[i][5] = -1;
                    if (pageTableCLOCK[i][4] == 1) {
                        moverPuntero();
                        pageTableCLOCK[i][4] = -1;
                    }
                }
                else {
                    char confirmacion;
                    cout<<" Se realizaron cambios en esta pagina, desea guardar los cambios(y/n): ";
                    cin>>confirmacion;
                    if (confirmacion == 'y') {
                            ofstream save;
                            save.open("disk/bloque" + to_string(pageTableCLOCK[i][1]) + ".bin", ios::out | ios::binary);
                            save.write(bufferPool[pageTableCLOCK[i][0]].data, diskController->sizeBloque);
                            save.close();
                        diskController->bloqueASector(pageTableCLOCK[i][1]);
                        cout<<"Se escribio la informacion del bloque "<<pageTableCLOCK[i][1]<<" correctamente en el disco."<<endl;
                        pageTableCLOCK[i][1] = -1;
                        pageTableCLOCK[i][2] = -1;
                        pageTableCLOCK[i][3] = -1;
                        pageTableCLOCK[i][5] = -1;
                        if (pageTableCLOCK[i][4] == 1) {
                            moverPuntero();
                            pageTableCLOCK[i][4] = -1;
                        }
                    }
                    else {
                        pageTableCLOCK[i][1] = -1;
                        pageTableCLOCK[i][2] = -1;
                        pageTableCLOCK[i][3] = -1;
                        pageTableCLOCK[i][5] = -1;
                        if (pageTableCLOCK[i][4] == 1) {
                            moverPuntero();
                            pageTableCLOCK[i][4] = -1;
                        }
                    }
                }
                return;
            }
        }
        cout<<"La pagina que desea limpiar del bufferPool no se encuentra en el ahora mismo."<<endl;
    }

    void PinCountACero(int pageId){
        for (int i=0; i<(int)pageTableCLOCK.size(); i++){
            if(pageTableCLOCK[i][1]==pageId){
                pageTableCLOCK[i][5] = 0;
                return;
            }
        }
        cout<<"La pagina no se encuentra en el ahora mismo."<<endl;
    }


    void showpageTableLRU(){
        cout<<"\n LRU tabla \n";
        for (int i=0; i<pageTableLRU.size(); i++){
            for (int j=0; j<pageTableLRU[0].size(); j++){
                cout<<pageTableLRU[i][j]<<"\t";
            }
            cout<<"\n";
        }
        cout<<"\n";
    }

    void showpageTableCLOCK(){
        cout<<"\n CLOCK tabla \n";
        cout<<"frame\tpage\tDB\tRefB\tPtr\tPinC\n";
        for (int i=0; i<pageTableCLOCK.size(); i++){
            for (int j=0; j<pageTableCLOCK[0].size(); j++){
                cout<<pageTableCLOCK[i][j]<<"\t";
            }
            cout<<"\n";
        }
        cout<<"\n";
    }
    void showpageTableMRU(){
        cout<<"\n MRU tabla \n";
        for (int i=0; i<pageTableMRU.size(); i++){
            for (int j=0; j<pageTableMRU[0].size(); j++){
                cout<<pageTableMRU[i][j]<<"\t";
            }
            cout<<"\n";
        }
        cout<<"\n";
    }
};


