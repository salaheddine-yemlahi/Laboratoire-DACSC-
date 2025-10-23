#ifndef MAINWINDOWCLIENTCONSULTATIONBOOKER_H
#define MAINWINDOWCLIENTCONSULTATIONBOOKER_H

#include <QMainWindow>
#include <string>
using namespace std;


typedef struct{
    int id_doctor;
    char first_name_doctor[20];
    char last_name_doctor[20];
}DOCTOR;

typedef struct 
{ 
    char nom[20];
    char prenom[20];
    int numeroPatient;
    bool nouveauPatient; 
} PATIENT;

typedef struct{
    int id_specialite;
    char nom_specialite[20];
}SPECIALITE;

typedef struct 
{
    char nom[50];
    char nomSpecialite[40];
    char dateDebut[10];
    char dateFin[10];
} RECHERCHE;

typedef struct {
    int typeMessage;
    int taille;
} TYPE;
/*
* 1 : IDENTIFIER
* 2 : RECHERCHER
*/
typedef struct{
    int idConsultation;
    char nomSpecialite[40];
    char nomMedecin[20];
    char prenomMedecin[20];
    char dateConsultation[20];
    char hourConsultation[5];
} REPONSE_RECHERCHE;

typedef struct {
    int id_consultation;
    int id_patient;
    char raison_consultation[100];
} BOOK_CONSULTATION;
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindowClientConsultationBooker; }
QT_END_NAMESPACE

class MainWindowClientConsultationBooker : public QMainWindow
{
    Q_OBJECT

public:
    int client;
    std::vector<REPONSE_RECHERCHE> tabReponses;
    MainWindowClientConsultationBooker(char* ip, char* port, QWidget *parent = nullptr);
    ~MainWindowClientConsultationBooker();

    void addTupleTableConsultations(int id, string specialty, string doctor, string date, string hour);
    void clearTableConsultations();
    int getSelectionIndexTableConsultations() const;

    void addComboBoxSpecialties(string specialty);
    string getSelectionSpecialty() const;
    void clearComboBoxSpecialties();

    void addComboBoxDoctors(string doctor);

    string getSelectionDoctor() const;
    void clearComboBoxDoctors();

    string getLastName() const;
    string getFirstName() const;
    int getPatientId() const;
    bool isNewPatientSelected() const;
    string getStartDate() const;
    string getEndDate() const;
    void setLastName(string value);
    void setFirstName(string value);
    void setPatientId(int value);
    void setNewPatientChecked(bool state);
    void setStartDate(string date);
    void setEndDate(string date);

    void loginOk();
    void logoutOk();

    void dialogMessage(const string& title,const string& message);
    void dialogError(const string& title,const string& message);
    string dialogInputText(const string& title,const string& question);
    int dialogInputInt(const string& title,const string& question);


private slots:
    void on_pushButtonLogin_clicked();
    void on_pushButtonLogout_clicked();
    void on_pushButtonRechercher_clicked();
    void on_pushButtonReserver_clicked();

private:
    Ui::MainWindowClientConsultationBooker *ui;
};
#endif // MAINWINDOWCLIENTCONSULTATIONBOOKER_H
