#include<stdio.h>
#include<stdlib.h>
#include<math.h>
    struct culori
    {
        float det;
        unsigned char CR,CB,CG;
        int fereastra;
    };
void Citire(char fisier_intrare[], unsigned char **header , unsigned char **PixelB, unsigned char **PixelG, unsigned char **PixelR, unsigned int *latime, unsigned int *inaltime, unsigned int *padding)
{
    unsigned int i;
    FILE *fin=fopen(fisier_intrare,"rb");

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
	fclose(fin);
}
void Afisare(char iesire[], unsigned char *header , unsigned char *PixelB, unsigned char *PixelG, unsigned char *PixelR, unsigned int latime, unsigned int inaltime, char padding)
{
    FILE *fout=fopen(iesire,"wb");
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
                 unsigned char c=0;
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
	fclose(fout);
}
void matching(unsigned char *PixelB, unsigned char *PixelG,unsigned char *PixelR,unsigned char *SablonB,unsigned char *SablonG,unsigned char *SablonR, double prag, unsigned int latime, unsigned int inaltime, unsigned int latimeS,unsigned int inaltimeS, float **corelatii)
{
    unsigned char aux;
    int i,j,k;
    for(i = 0; i <latime*inaltime; i++)                                      ///Fac imaginile Greyscale
	{
	        aux = 0.299*PixelR[i] + 0.587*PixelG[i] + 0.114*PixelB[i];
	        PixelR[i]=PixelG[i]=PixelB[i]=aux;
	}
	    for(i = 0; i <latimeS*inaltimeS; i++)
	{
	        aux = 0.299*SablonR[i] + 0.587*SablonG[i] + 0.114*SablonB[i];
	        SablonR[i]=SablonG[i]=SablonB[i]=aux;
	}

	double Sbar=0,SigmaS=0;
    for(i = 0; i <latimeS*inaltimeS; i++)              ///Media Intensitatilor Greyscale in sablon
	{
	    Sbar=Sbar+SablonR[i];
	}
	Sbar=Sbar/(latimeS*inaltimeS);
	double n=latimeS*inaltimeS;

    for(i = 0; i <latimeS*inaltimeS; i++)              ///Calculul deviatiei in sablon
	{
	    SigmaS=(SablonR[i]-Sbar)*(SablonR[i]-Sbar)+SigmaS;
	}
	SigmaS=SigmaS*(1/(n-1));
	SigmaS=sqrt(SigmaS);



	double cor=0,fbar=0,SigmaF=0;
	for(i=0;i<latime*inaltime-latime*inaltimeS;i++)     ///Parcurg imaginea normala cu centrarea sablonului din coltul stanga sus
    {
        cor=0;
        for(k=0;k<inaltimeS;k++)
        {
            for(j=i;j<i+latimeS;j++)                         ///Media intenstiatilor Greyscale in fereasatra
            {
                fbar=fbar+PixelB[j+k*latime];
            }
        }
        fbar=fbar/n;


        for(k=0;k<inaltimeS;k++)
        {
            for(j=i;j<i+latimeS;j++)                         ///Calculul deviatiei in fereastra
            {
                SigmaF=(PixelB[j+k*latime]-fbar)*(PixelB[j+k*latime]-fbar)+SigmaF;
            }
        }
        SigmaF=SigmaF*(1/(n-1));
        SigmaF=sqrt(SigmaF);


        int z=0;
        for(k=0;k<inaltimeS;k++)                         ///Calculul Corelatiei
        {
            for(j=i;j<i+latimeS;j++)
            {
                cor=(1/(SigmaF*SigmaS))*(PixelB[j+k*latime]-fbar)*(SablonB[z]-Sbar)+cor;
                z++;
            }
        }
        cor=cor*(1/n);
        if(cor>prag)
        {
            (*corelatii)[i]=cor;
        }
    }
}
void colorezfereastra(unsigned char **PixelB, unsigned char **PixelG, unsigned char **PixelR, int fereastra, unsigned int latimeS, unsigned int inaltimeS,unsigned int latime,unsigned int inaltime, unsigned char R,unsigned char G,unsigned char B)
{
    int k,j;
    for(k=0;k<=inaltimeS;k++)
    {
        for(j=fereastra;j<=fereastra+latimeS;j++)                         ///Media intenstiatilor Greyscale in fereasatra
        {

                if(k==0 || k==inaltimeS || j==fereastra || j==fereastra+latimeS)
                {
                    (*PixelB)[j+k*latime]=B;
                    (*PixelG)[j+k*latime]=G;
                    (*PixelR)[j+k*latime]=R;
                }
        }
    }
}
int cmp(const void *x, const void *y)
{
    const struct culori * dx = (const struct culori *) x;
    const struct culori * dy = (const struct culori *) y;
    if(dx->det > dy->det)
        return -1;

    if(dx->det < dy->det)
        return 1;


    return 0;
}
void non_maxim(struct culori **x, int n,unsigned int latime, unsigned int inaltime, unsigned int latimeS, unsigned int inaltimeS)
{
    int i,j,k,z;
    for(i=0;i<n;i++)
    {
        for(j=0;j<n;j++)
        {
            if((*x)[i].det > (*x)[j].det && i!=j && (*x)[j].det!=0)
            {
                int a[latime*inaltime];
                for(k=0;k<latime*inaltime;k++)
                {
                    a[k]=0;
                }
                for(k=0;k<inaltimeS;k++)
                {
                    for(z=(*x)[i].fereastra;z<(*x)[i].fereastra+latimeS;z++)
                    {
                        a[z+k*latime]++;
                    }
                }
                for(k=0;k<inaltimeS;k++)
                {
                    for(z=(*x)[j].fereastra;z<(*x)[j].fereastra+latimeS;z++)
                    {
                        a[z+k*latime]++;
                    }
                }
                float cont=0;
                for(k=0;k<latime*inaltime;k++)
                {
                    if(a[k]==2)
                        cont=cont+1;
                }
                float sup;
                sup=cont/(latimeS*inaltimeS-cont);
                if(sup>0.2)
                {
                    (*x)[j].det=0;
                }
            }
        }
    }
}
int main ()
{
    unsigned int latime,inaltime,padding,latimeS,inaltimeS,paddingS,i;
    unsigned char *PixelB,*PixelG,*PixelR;
    unsigned char *SablonB, *SablonG, *SablonR;
    unsigned char *header, *headerS;
    double ps;
    float *corelatii;
    char fisier_intrare[30], fisier_iesire[30];
    int sablon, nrsabloane;
    printf("Introduce-ti fisierul de intrare: ");
    scanf("%s",fisier_intrare);
    printf("Introduce-ti fisierul de iesire: ");
    scanf("%s",fisier_iesire);
    printf("Introduce-ti pragul ps= ");
    scanf("%lf",&ps);
    printf("introduce-ti numarul de sabloane= ");
    scanf("%d",&nrsabloane);
    printf("introduce-ti sabloanele (0-9)= ");


    Citire(fisier_intrare,&header,&PixelB,&PixelG,&PixelR,&latime,&inaltime,&padding);
    corelatii=calloc(latime*inaltime+1, sizeof(float));


    struct culori *determinare;
    determinare=calloc(latime*inaltime,sizeof(struct culori));

    while(nrsabloane>0)
    {
        scanf("%d",&sablon);
        if(sablon==0)
        {
        Citire("cifra0.bmp",&headerS,&SablonB,&SablonG,&SablonR,&latimeS,&inaltimeS,&paddingS);
        matching(PixelB,PixelG,PixelR,SablonB,SablonG,SablonR,ps,latime,inaltime,latimeS,inaltimeS,&corelatii);
        for(i=0;i<=latime*inaltime;i++)
        {
            if(corelatii[i]!=0 && determinare[i].det<corelatii[i])
            {
                determinare[i].det=corelatii[i];
                determinare[i].CR=255;
                determinare[i].CG=0;
                determinare[i].CB=0;
                determinare[i].fereastra=i;
            }
        }
        }
        if(sablon==1)
        {
            Citire("cifra1.bmp",&headerS,&SablonB,&SablonG,&SablonR,&latimeS,&inaltimeS,&paddingS);
            matching(PixelB,PixelG,PixelR,SablonB,SablonG,SablonR,ps,latime,inaltime,latimeS,inaltimeS,&corelatii);
            for(i=0;i<=latime*inaltime;i++)
            {
                if(corelatii[i]!=0 && determinare[i].det<corelatii[i])
                {
                    determinare[i].det=corelatii[i];
                    determinare[i].CR=255;
                    determinare[i].CG=255;
                    determinare[i].CB=0;
                    determinare[i].fereastra=i;
                }
            }
        }
        if(sablon==2)
        {
            Citire("cifra2.bmp",&headerS,&SablonB,&SablonG,&SablonR,&latimeS,&inaltimeS,&paddingS);
            matching(PixelB,PixelG,PixelR,SablonB,SablonG,SablonR,ps,latime,inaltime,latimeS,inaltimeS,&corelatii);
            for(i=0;i<=latime*inaltime;i++)
            {
                if(corelatii[i]!=0 && determinare[i].det<corelatii[i])
                {
                    determinare[i].det=corelatii[i];
                    determinare[i].CR=0;
                    determinare[i].CG=255;
                    determinare[i].CB=0;
                    determinare[i].fereastra=i;
                }
            }
        }
        if(sablon==3)
        {
            Citire("cifra3.bmp",&headerS,&SablonB,&SablonG,&SablonR,&latimeS,&inaltimeS,&paddingS);
            matching(PixelB,PixelG,PixelR,SablonB,SablonG,SablonR,ps,latime,inaltime,latimeS,inaltimeS,&corelatii);
            for(i=0;i<=latime*inaltime;i++)
            {
                if(corelatii[i]!=0 && determinare[i].det<corelatii[i])
                {
                    determinare[i].det=corelatii[i];
                    determinare[i].CR=0;
                    determinare[i].CG=255;
                    determinare[i].CB=255;
                    determinare[i].fereastra=i;
                }
            }
        }
        if(sablon==4)
        {
            Citire("cifra4.bmp",&headerS,&SablonB,&SablonG,&SablonR,&latimeS,&inaltimeS,&paddingS);
            matching(PixelB,PixelG,PixelR,SablonB,SablonG,SablonR,ps,latime,inaltime,latimeS,inaltimeS,&corelatii);
            for(i=0;i<=latime*inaltime;i++)
            {
                if(corelatii[i]!=0 && determinare[i].det<corelatii[i])
                {
                    determinare[i].det=corelatii[i];
                    determinare[i].CR=255;
                    determinare[i].CG=0;
                    determinare[i].CB=255;
                    determinare[i].fereastra=i;
                }
            }
        }
        if(sablon==5)
        {
            Citire("cifra5.bmp",&headerS,&SablonB,&SablonG,&SablonR,&latimeS,&inaltimeS,&paddingS);
            matching(PixelB,PixelG,PixelR,SablonB,SablonG,SablonR,ps,latime,inaltime,latimeS,inaltimeS,&corelatii);
            for(i=0;i<=latime*inaltime;i++)
            {
                if(corelatii[i]!=0 && determinare[i].det<corelatii[i])
                {
                    determinare[i].det=corelatii[i];
                    determinare[i].CR=0;
                    determinare[i].CG=0;
                    determinare[i].CB=255;
                    determinare[i].fereastra=i;
                }
            }
        }
        if(sablon==6)
        {
            Citire("cifra6.bmp",&headerS,&SablonB,&SablonG,&SablonR,&latimeS,&inaltimeS,&paddingS);
            matching(PixelB,PixelG,PixelR,SablonB,SablonG,SablonR,ps,latime,inaltime,latimeS,inaltimeS,&corelatii);
            for(i=0;i<=latime*inaltime;i++)
            {
                if(corelatii[i]!=0 && determinare[i].det<corelatii[i])
                {
                    determinare[i].det=corelatii[i];
                    determinare[i].CR=192;
                    determinare[i].CG=192;
                    determinare[i].CB=192;
                    determinare[i].fereastra=i;
                }
            }
        }
        if(sablon==7)
        {
            Citire("cifra7.bmp",&headerS,&SablonB,&SablonG,&SablonR,&latimeS,&inaltimeS,&paddingS);
            matching(PixelB,PixelG,PixelR,SablonB,SablonG,SablonR,ps,latime,inaltime,latimeS,inaltimeS,&corelatii);
            for(i=0;i<=latime*inaltime;i++)
            {
                if(corelatii[i]!=0 && determinare[i].det<corelatii[i])
                {
                    determinare[i].det=corelatii[i];
                    determinare[i].CR=255;
                    determinare[i].CG=140;
                    determinare[i].CB=0;
                    determinare[i].fereastra=i;
                }
            }
        }
        if(sablon==8)
        {
            Citire("cifra8.bmp",&headerS,&SablonB,&SablonG,&SablonR,&latimeS,&inaltimeS,&paddingS);
            matching(PixelB,PixelG,PixelR,SablonB,SablonG,SablonR,ps,latime,inaltime,latimeS,inaltimeS,&corelatii);
            for(i=0;i<=latime*inaltime;i++)
            {
                if(corelatii[i]!=0 && determinare[i].det<corelatii[i])
                {
                    determinare[i].det=corelatii[i];
                    determinare[i].CR=128;
                    determinare[i].CG=0;
                    determinare[i].CB=128;
                    determinare[i].fereastra=i;
                }
            }
        }
        if(sablon==9)
        {
            Citire("cifra0.bmp",&headerS,&SablonB,&SablonG,&SablonR,&latimeS,&inaltimeS,&paddingS);
            matching(PixelB,PixelG,PixelR,SablonB,SablonG,SablonR,ps,latime,inaltime,latimeS,inaltimeS,&corelatii);
            for(i=0;i<=latime*inaltime;i++)
            {
                if(corelatii[i]!=0 && determinare[i].det<corelatii[i])
                {
                    determinare[i].det=corelatii[i];
                    determinare[i].CR=128;
                    determinare[i].CG=0;
                    determinare[i].CB=0;
                    determinare[i].fereastra=i;
                }
            }
        }
        nrsabloane--;

    }
   /// qsort(determinare,latime*inaltime,sizeof(struct culori *),cmp);               ///Sortarea elementelor
    non_maxim(&determinare,latime*inaltime,latime,inaltime,latimeS,inaltimeS);
    for(i=0;i<=latime*inaltime;i++)
    {
        if(determinare[i].det!=0)
        {
            colorezfereastra(&PixelB,&PixelG,&PixelR,determinare[i].fereastra,latimeS,inaltimeS,latime,inaltime,determinare[i].CR,determinare[i].CG,determinare[i].CB);
        }
    }

    Afisare(fisier_iesire,header,PixelB,PixelG,PixelR,latime,inaltime,padding);
    free(PixelB);
    free(PixelG);
    free(PixelR);
    free(header);
    free(corelatii);
    free(SablonB);
    free(SablonG);
    free(SablonR);
    free(headerS);
    free(determinare);
    return 0;
}
