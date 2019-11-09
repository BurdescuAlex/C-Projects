#include<stdio.h>
#include<stdlib.h>

void Citire( FILE * fin, unsigned char **header , unsigned char **PixelB, unsigned char **PixelG, unsigned char **PixelR, unsigned int *latime, unsigned int *inaltime, unsigned int *padding)
{
    unsigned int i;

///   fseek(fin, 2, SEEK_SET);                 ///Citire Dimensiune
///   fread(&dim,sizeof(unsigned int),1,fin);


    fseek(fin, 18, SEEK_SET);                ///Citire Latime
    fread(latime, sizeof(unsigned int), 1, fin);


    fseek(fin, 22, SEEK_SET);                ///Citire Inaltime
    fread(inaltime, sizeof(unsigned int), 1, fin);


    fseek(fin,0,SEEK_SET);
    *header=malloc(54);                       ///Citire Header
    for(i=0;i<54;i++)
    {
        fread(&(*header)[i], sizeof(char),1,fin);
    }



    *padding=(4-(3*(*latime))% 4)%4;             ///Calcul Nr Octeti Padding
    char *citescpadding;


    citescpadding=malloc(*padding);            ///Citire Octeti Imagine si inlaturarea Paddingului
    *PixelB=malloc((*latime)*(*inaltime));
    *PixelG=malloc((*latime)*(*inaltime));
    *PixelR=malloc((*latime)*(*inaltime));

    fseek(fin, 54, SEEK_SET);
	for(i = 0; i < ((*latime)*(*inaltime)); i++)
	{
            if(i%(*latime)==0 && i!=0 && (*padding)!=0)
            {
                fread(citescpadding,sizeof(char),*padding,fin);
            }
		    fread(&(*PixelB)[i],sizeof(char),1, fin);
		    fread(&(*PixelG)[i],sizeof(char),1, fin);
		    fread(&(*PixelR)[i],sizeof(char),1, fin);
	}

	free(citescpadding);
}
void Afisare(FILE *fout, unsigned char *header , unsigned char *PixelB, unsigned char *PixelG, unsigned char *PixelR, unsigned int latime, unsigned int inaltime, char padding)
{
    fseek(fout, 0, SEEK_SET);
    int i;
    for(i = 0; i <54; i++)                        ///Afisare Header
	{
            fwrite(&header[i],sizeof(char),1,fout);
	}


	fseek(fout, 54, SEEK_SET);
    for(i = 0; i < inaltime*latime; i++)              ///Afisare imagine liniarizata+padding
	{
             if((i+1)%latime==0 && padding!=0)
             {
                 int j=padding;
                 char c=0;
                 while(j>0)
                 {
                 fwrite(&c,sizeof(char),1,fout);
                 j--;
                 }
             }
	         fwrite(&PixelB[i],sizeof(char),1,fout);
	         fwrite(&PixelG[i],sizeof(char),1,fout);
	         fwrite(&PixelR[i],sizeof(char),1,fout);
	}
}
void XORSHIFT32(unsigned int **R, unsigned int n, FILE *key)
{
    fseek(key,0,SEEK_SET);
    unsigned int seed,i;
    fscanf(key,"%d",&seed);
    *R=malloc(n*sizeof(int));

    (*R)[0]=seed;
    for(i=1;i<n;i++)
    {
        seed=seed^seed<<13;
        seed=seed^seed>>17;
        seed=seed^seed<<5;
        (*R)[i]=seed;
    }
}
void Durstenfeld(unsigned int **P, unsigned int n, unsigned int *R)
{
    int k,r,aux;

    *P=malloc(n*sizeof(int));
    for(k=0;k<n;k++)
    {
        (*P)[k]=k;
    }
    for(k=n-1;k>0;k--)
    {
        r=R[n-k]%(k+1);                         ///Utilizam numerele R[1] - R[lungime*latime-1] ca numere random in cadrul algoritmului
        aux=(*P)[r];
        (*P)[r]=(*P)[k];
        (*P)[k]=aux;
    }
}
void Criptare(char nume_imagine_citire [], char nume_imagine_afisare[], char nume_cheie[])
{
    unsigned char *header, *PixelB, *PixelG, *PixelR , *Pb, *Pg, *Pr;
    unsigned int latime,inaltime,i;
    unsigned int padding, *R , *P, *A;

       FILE *fin=fopen(nume_imagine_citire,"rb");
       FILE *fout=fopen(nume_imagine_afisare,"wb");
       FILE *key=fopen(nume_cheie,"r");

    Citire(fin,&header,&PixelB,&PixelG,&PixelR,&latime,&inaltime,&padding);
    XORSHIFT32(&R,2*latime*inaltime,key);
    Durstenfeld(&P,latime*inaltime,R);



        Pb=malloc(latime*inaltime*sizeof(char));
        Pg=malloc(latime*inaltime*sizeof(char));
        Pr=malloc(latime*inaltime*sizeof(char));
        A=malloc(latime*inaltime*sizeof(int));
        for(i=0;i<latime*inaltime;i++)                           ///Realizarea permutarii si copierea imaginii
        {
            A[P[i]]=i;
            Pb[i]=PixelB[i];
            Pg[i]=PixelG[i];
            Pr[i]=PixelR[i];
        }
        for(i = 0; i <latime*inaltime; i++)                      ///Interschimbarea Pixelilor confrom Permutarii
        {
            PixelB[i]=Pb[A[i]];
            PixelG[i]=Pg[A[i]];
            PixelR[i]=Pr[A[i]];
        }
        free(Pb);
        free(Pg);
        free(Pr);
        free(A);


	union bytes
	{
        int x;
        int v[4];
    }data;

	data.x=R[inaltime*latime];

	int SV;                                             ///Citirea SV si criptarea primului pixel
	fscanf(key,"%d",&SV);
	PixelB[0]=SV^PixelB[0]^data.v[0];
	PixelG[0]=SV^PixelG[0]^data.v[1];
	PixelR[0]=SV^PixelR[0]^data.v[2];

    for(i =1; i<latime*inaltime; i++)                            ///Criptarea restului Pixelilor confrom formulei
    {
        data.x=R[inaltime*latime+i];
        PixelB[i]=PixelB[i-1]^PixelB[i]^data.v[0];
        PixelG[i]=PixelG[i-1]^PixelG[i]^data.v[1];
        PixelR[i]=PixelR[i-1]^PixelR[i]^data.v[2];
    }


    Afisare(fout,header,PixelB,PixelG,PixelR,latime,inaltime,padding);
    free(R);
    free(P);
    free(header);
	free(PixelB);
	free(PixelG);
	free(PixelR);
	        fclose(fin);
        fclose(fout);
        fclose(key);
}
void Decriptare(char nume_imagine_citire [], char nume_imagine_afisare[], char nume_cheie[])
{
    unsigned char *header, *PixelB, *PixelG, *PixelR , *Pb, *Pg, *Pr;
    unsigned int latime,inaltime,i;
    unsigned int padding, *R , *P, *A;
       FILE *finc=fopen(nume_imagine_citire,"rb");
       FILE *foutc=fopen(nume_imagine_afisare,"wb");
       FILE *key=fopen(nume_cheie,"r");

    Citire(finc,&header,&PixelB,&PixelG,&PixelR,&latime,&inaltime,&padding);
    XORSHIFT32(&R,2*latime*inaltime,key);
    Durstenfeld(&P,latime*inaltime,R);


	union bytes
	{
        int x;
        int v[4];
    }data;

	data.x=R[inaltime*latime];

	int SV;                                             ///Citirea SV
	fscanf(key,"%d",&SV);
    for(i =latime*inaltime-1; i>0; i--)                            ///Decriptarea pixelilor recursiv confrom formulei - invers
    {
        data.x=R[inaltime*latime+i];
        PixelB[i]=PixelB[i-1]^PixelB[i]^data.v[0];
        PixelG[i]=PixelG[i-1]^PixelG[i]^data.v[1];
        PixelR[i]=PixelR[i-1]^PixelR[i]^data.v[2];
    }
	PixelB[0]=SV^PixelB[0]^data.v[0];                 ///Decriptarea primului pixel
	PixelG[0]=SV^PixelG[0]^data.v[1];
	PixelR[0]=SV^PixelR[0]^data.v[2];



    Pb=malloc(latime*inaltime*sizeof(char));
    Pg=malloc(latime*inaltime*sizeof(char));
    Pr=malloc(latime*inaltime*sizeof(char));
    A=malloc(latime*inaltime*sizeof(int));
    for(i=0;i<latime*inaltime;i++)                           ///Realizarea permutarii si copierea imaginii
    {
        A[P[i]]=i;
        Pb[i]=PixelB[i];
        Pg[i]=PixelG[i];
        Pr[i]=PixelR[i];
    }
    for(i = 0; i <latime*inaltime; i++)                      ///Interschimbarea Pixelilor confrom Permutarii Inverse
    {
        PixelB[A[i]]=Pb[i];
        PixelG[A[i]]=Pg[i];
        PixelR[A[i]]=Pr[i];
    }
    free(Pb);
    free(Pg);
    free(Pr);
    free(A);



    Afisare(foutc,header,PixelB,PixelG,PixelR,latime,inaltime,padding);
    free(R);
    free(P);
    free(header);
	free(PixelB);
	free(PixelG);
	free(PixelR);

            fclose(key);
            fclose(finc);
            fclose(foutc);
}
void chi(char nume_imagine_citire[])
{
    FILE *fout=fopen("chi_test.txt","w");
    FILE *fin=fopen(nume_imagine_citire,"rb");

    int latime,inaltime,i,padding;
    double chiB=0,chiG=0,chiR=0,fbar=0;
    unsigned char f=0;
    int *frecventaB, *frecventaG, *frecventaR;

    frecventaB=(int*)calloc(256,sizeof(int));
    frecventaG=(int*)calloc(256,sizeof(int));
    frecventaR=(int*)calloc(256,sizeof(int));

    fseek(fin, 18, SEEK_SET);                ///Citire Latime
    fread(&latime, sizeof(unsigned int), 1, fin);


    fseek(fin, 22, SEEK_SET);                ///Citire Inaltime
    fread(&inaltime, sizeof(unsigned int), 1, fin);

    padding=(4-(3*latime)% 4)%4;             ///Calcul Nr Octeti Padding
    char *citescpadding;


    citescpadding=malloc(padding);
    fseek(fin, 54, SEEK_SET);
    fbar=latime*inaltime/256.0;
	for(i = 0; i < latime*inaltime; i++)
	{
            if(i%latime==0 && i!=0 && padding!=0)
            {
                fread(citescpadding,sizeof(char),padding,fin);
            }
		    fread(&f,sizeof(char),1, fin);
		    frecventaB[f]=frecventaB[f]+1;
		    fread(&f,sizeof(char),1, fin);
		    frecventaG[f]=frecventaG[f]+1;
            fread(&f,sizeof(char),1, fin);
            frecventaR[f]=frecventaR[f]+1;
	}
	for(i=0;i<=255;i++)
    {
		    chiB=(frecventaB[i]-fbar)*(frecventaB[i]-fbar)/fbar+chiB;
		    chiG=(frecventaG[i]-fbar)*(frecventaG[i]-fbar)/fbar+chiG;
		    chiR=(frecventaR[i]-fbar)*(frecventaR[i]-fbar)/fbar+chiR;
    }


	fprintf(fout,"%.2lf %.2lf %.2lf ",chiR,chiG,chiB);
	free(citescpadding);
	fclose(fout);
    fclose(fin);
    free(frecventaB);
    free(frecventaG);
    free(frecventaR);
}
int main ()
{
    char nume_imagine_citire[20];
    char nume_imagine_afisare[20];
    char nume_cheie[20];

    int p;
    printf("1-Criptare\n2-Decriptare\n3-Chi-test\nComanda=");
    scanf("%d",&p);

    if(p==1)       ///Criptare
    {
        printf("Nume fisier de intrare: ");
        scanf("%s",nume_imagine_citire);
        printf("\nNume fisier de iesire: ");
        scanf("%s",nume_imagine_afisare);
        printf("\nNume fisier cheie secreta: ");
        scanf("%s",nume_cheie);


       Criptare(nume_imagine_citire,nume_imagine_afisare,nume_cheie);

    }
    if(p==2)  ///Decriptare
    {
        printf("Nume fisier de intrare: ");
        scanf("%s",nume_imagine_citire);
        printf("\nNume fisier de iesire: ");
        scanf("%s",nume_imagine_afisare);
        printf("\nNume fisier cheie secreta: ");
        scanf("%s",nume_cheie);

       Decriptare(nume_imagine_citire,nume_imagine_afisare,nume_cheie);

    }
    if(p==3)       ///Test chi
    {
        printf("Nume fisier de intrare: ");
        scanf("%s",nume_imagine_citire);
        chi(nume_imagine_citire);
    }


return 0;
}
