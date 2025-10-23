#include "mainwindowclientconsultationbooker.h"
#include "ui_mainwindowclientconsultationbooker.h"
#include <QInputDialog>
#include <QMessageBox>
#include <iostream>
#include "LibrairieClient.h"
using namespace std;

MainWindowClientConsultationBooker::MainWindowClientConsultationBooker(char* ip, char* port, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindowClientConsultationBooker)
    , tabReponses(20)
{
    client = connecterServeur(ip, atoi(port));
    if (client < 0) {
        printf("Erreur: impossible de connecter avec le serveur\n");
        return;
    }
    printf("Connexion réussie!\n");
    ui->setupUi(this);
    logoutOk();
    


    // Configuration de la table des employes (Personnel Garage)
    ui->tableWidgetConsultations->setColumnCount(5);
    ui->tableWidgetConsultations->setRowCount(0);
    QStringList labelsTableConsultations;
    labelsTableConsultations << "Id" << "Spécialité" << "Médecin" << "Date" << "Heure";
    ui->tableWidgetConsultations->setHorizontalHeaderLabels(labelsTableConsultations);
    ui->tableWidgetConsultations->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableWidgetConsultations->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidgetConsultations->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidgetConsultations->horizontalHeader()->setVisible(true);
    ui->tableWidgetConsultations->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidgetConsultations->verticalHeader()->setVisible(false);
    ui->tableWidgetConsultations->horizontalHeader()->setStyleSheet("background-color: lightyellow");
    int columnWidths[] = {40, 150, 200, 150, 100};
    for (int col = 0; col < 5; ++col)
        ui->tableWidgetConsultations->setColumnWidth(col, columnWidths[col]);

    // Exemples d'utilisation (à supprimer)
    //this->addTupleTableConsultations(1,"Neurologie","Martin Claire","2025-10-01", "09:00");
    //this->addTupleTableConsultations(2,"Cardiologie","Lemoine Bernard","2025-10-06", "10:15");
    //this->addTupleTableConsultations(3,"Dermatologie","Maboul Paul","2025-10-23", "14:30");

    //this->addComboBoxSpecialties("--- TOUTES ---");
    // this->addComboBoxSpecialties("Dermatologie");
    // this->addComboBoxSpecialties("Cardiologie");

    //this->addComboBoxDoctors("--- TOUS ---");
    //this->addComboBoxDoctors("Martin Claire");
    //this->addComboBoxDoctors("Maboul Paul");
}

MainWindowClientConsultationBooker::~MainWindowClientConsultationBooker()
{
    // Fermer la connexion
    fermerSocket(client);
    delete ui;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions utiles Table des livres encodés (ne pas modifier) ////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindowClientConsultationBooker::addTupleTableConsultations(int id,
                                                                    string specialty,
                                                                    string doctor,
                                                                    string date,
                                                                    string hour)
{
    int nb = ui->tableWidgetConsultations->rowCount();
    nb++;
    ui->tableWidgetConsultations->setRowCount(nb);
    ui->tableWidgetConsultations->setRowHeight(nb-1,10);

    // id
    QTableWidgetItem *item = new QTableWidgetItem;
    item->setTextAlignment(Qt::AlignCenter);
    item->setText(QString::number(id));
    ui->tableWidgetConsultations->setItem(nb-1,0,item);

    // specialty
    item = new QTableWidgetItem;
    item->setTextAlignment(Qt::AlignCenter);
    item->setText(QString::fromStdString(specialty));
    ui->tableWidgetConsultations->setItem(nb-1,1,item);

    // doctor
    item = new QTableWidgetItem;
    item->setTextAlignment(Qt::AlignCenter);
    item->setText(QString::fromStdString(doctor));
    ui->tableWidgetConsultations->setItem(nb-1,2,item);

    // date
    item = new QTableWidgetItem;
    item->setTextAlignment(Qt::AlignCenter);
    item->setText(QString::fromStdString(date));
    ui->tableWidgetConsultations->setItem(nb-1,3,item);

    // hour
    item = new QTableWidgetItem;
    item->setTextAlignment(Qt::AlignCenter);
    item->setText(QString::fromStdString(hour));
    ui->tableWidgetConsultations->setItem(nb-1,4,item);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindowClientConsultationBooker::clearTableConsultations() {
    ui->tableWidgetConsultations->setRowCount(0);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
int MainWindowClientConsultationBooker::getSelectionIndexTableConsultations() const
{
    QModelIndexList list = ui->tableWidgetConsultations->selectionModel()->selectedRows();
    if (list.size() == 0) return -1;
    QModelIndex index = list.at(0);
    int ind = index.row();
    return ind;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions utiles des comboboxes (ne pas modifier) //////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindowClientConsultationBooker::addComboBoxSpecialties(string specialty) {
    ui->comboBoxSpecialties->addItem(QString::fromStdString(specialty));
}

string MainWindowClientConsultationBooker::getSelectionSpecialty() const {
    return ui->comboBoxSpecialties->currentText().toStdString();
}

void MainWindowClientConsultationBooker::clearComboBoxSpecialties() {
    ui->comboBoxSpecialties->clear();
    this->addComboBoxSpecialties("--- TOUTES ---");
}

void MainWindowClientConsultationBooker::addComboBoxDoctors(string doctor) {
    ui->comboBoxDoctors->addItem(QString::fromStdString(doctor));
}

string MainWindowClientConsultationBooker::getSelectionDoctor() const {
    return ui->comboBoxDoctors->currentText().toStdString();
}

void MainWindowClientConsultationBooker::clearComboBoxDoctors() {
    ui->comboBoxDoctors->clear();
    this->addComboBoxDoctors("--- TOUS ---");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonction utiles de la fenêtre (ne pas modifier) ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
string MainWindowClientConsultationBooker::getLastName() const {
    return ui->lineEditLastName->text().toStdString();
}

string MainWindowClientConsultationBooker::getFirstName() const {
    return ui->lineEditFirstName->text().toStdString();
}

int MainWindowClientConsultationBooker::getPatientId() const {
    return ui->spinBoxId->value();
}

void MainWindowClientConsultationBooker::setLastName(string value) {
    ui->lineEditLastName->setText(QString::fromStdString(value));
}

string MainWindowClientConsultationBooker::getStartDate() const {
    return ui->dateEditStartDate->date().toString("yyyy-MM-dd").toStdString();
}

string MainWindowClientConsultationBooker::getEndDate() const {
    return ui->dateEditEndDate->date().toString("yyyy-MM-dd").toStdString();
}

void MainWindowClientConsultationBooker::setFirstName(string value) {
    ui->lineEditFirstName->setText(QString::fromStdString(value));
}

void MainWindowClientConsultationBooker::setPatientId(int value) {
    if (value > 0) ui->spinBoxId->setValue(value);
}

bool MainWindowClientConsultationBooker::isNewPatientSelected() const {
    return ui->checkBoxNewPatient->isChecked();
}

void MainWindowClientConsultationBooker::setNewPatientChecked(bool state) {
    ui->checkBoxNewPatient->setChecked(state);
}

void MainWindowClientConsultationBooker::setStartDate(string date) {
    QDate qdate = QDate::fromString(QString::fromStdString(date), "yyyy-MM-dd");
    if (qdate.isValid()) ui->dateEditStartDate->setDate(qdate);
}

void MainWindowClientConsultationBooker::setEndDate(string date) {
    QDate qdate = QDate::fromString(QString::fromStdString(date), "yyyy-MM-dd");
    if (qdate.isValid()) ui->dateEditEndDate->setDate(qdate);
}

void MainWindowClientConsultationBooker::loginOk() {
    ui->lineEditLastName->setReadOnly(true);
    ui->lineEditFirstName->setReadOnly(true);
    ui->spinBoxId->setReadOnly(true);
    ui->checkBoxNewPatient->setEnabled(false);
    ui->pushButtonLogout->setEnabled(true);
    ui->pushButtonLogin->setEnabled(false);
    ui->pushButtonRechercher->setEnabled(true);
    ui->pushButtonReserver->setEnabled(true);



    TYPE type;
    type.typeMessage = 4;
    envoyerMessage(client, &type, sizeof(TYPE));

    int nbResultats;
    recevoirReponse(client, &nbResultats, sizeof(nbResultats));
    printf("Nombre de spécialités reçues : %d\n", nbResultats);
    this->clearComboBoxSpecialties();
    if (nbResultats > 0)
    {
        vector<SPECIALITE> tabReponses(nbResultats);

        recevoirReponse(client, tabReponses.data(), nbResultats * sizeof(SPECIALITE));
        for (int i = 0; i < nbResultats; i++)
        {
            printf("Spécialité %d : %d - %s\n", 
                i + 1, 
                tabReponses[i].id_specialite, 
                tabReponses[i].nom_specialite);

            this->addComboBoxSpecialties(tabReponses[i].nom_specialite);
        }
    }



    type.typeMessage = 5;
    envoyerMessage(client, &type, sizeof(TYPE));

    recevoirReponse(client, &nbResultats, sizeof(nbResultats));
    printf("Nombre de spécialités reçues : %d\n", nbResultats);
    this->clearComboBoxDoctors();
    if (nbResultats > 0)
    {
        vector<DOCTOR> tabReponses(nbResultats);

        recevoirReponse(client, tabReponses.data(), nbResultats * sizeof(DOCTOR));

        for (int i = 0; i < nbResultats; i++)
        {
            printf("Doctor %d : %d - %s %s\n", 
                i + 1, 
                tabReponses[i].id_doctor, 
                tabReponses[i].first_name_doctor,
                tabReponses[i].last_name_doctor);

            std::string nomComplet = std::string(tabReponses[i].first_name_doctor) + " " + tabReponses[i].last_name_doctor;
            this->addComboBoxDoctors(nomComplet.c_str());
        }

    }
}

void MainWindowClientConsultationBooker::logoutOk() {
    ui->lineEditLastName->setReadOnly(false);
    setLastName("");
    ui->lineEditFirstName->setReadOnly(false);
    setFirstName("");
    ui->spinBoxId->setReadOnly(false);
    setPatientId(0);
    ui->checkBoxNewPatient->setEnabled(true);
    setNewPatientChecked(false);
    ui->pushButtonLogout->setEnabled(false);
    ui->pushButtonLogin->setEnabled(true);
    ui->pushButtonRechercher->setEnabled(false);
    ui->pushButtonReserver->setEnabled(false);
    setStartDate("2025-09-15");
    setEndDate("2025-12-31");
    clearComboBoxDoctors();
    clearComboBoxSpecialties();
    clearTableConsultations();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions permettant d'afficher des boites de dialogue (ne pas modifier) ///////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindowClientConsultationBooker::dialogMessage(const string& title,const string& message) {
   QMessageBox::information(this,QString::fromStdString(title),QString::fromStdString(message));
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindowClientConsultationBooker::dialogError(const string& title,const string& message) {
   QMessageBox::critical(this,QString::fromStdString(title),QString::fromStdString(message));
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
string MainWindowClientConsultationBooker::dialogInputText(const string& title,const string& question) {
    return QInputDialog::getText(this,QString::fromStdString(title),QString::fromStdString(question)).toStdString();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
int MainWindowClientConsultationBooker::dialogInputInt(const string& title,const string& question) {
    return QInputDialog::getInt(this,QString::fromStdString(title),QString::fromStdString(question));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions gestion des boutons (TO DO) //////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindowClientConsultationBooker::on_pushButtonLogin_clicked()
{
    PATIENT patient;
    TYPE type;

    strncpy(patient.nom, this->getLastName().c_str(), sizeof(patient.nom)-1);
    patient.nom[sizeof(patient.nom)-1] = '\0';

    strncpy(patient.prenom, this->getFirstName().c_str(), sizeof(patient.prenom)-1);
    patient.prenom[sizeof(patient.prenom)-1] = '\0';

    patient.numeroPatient = this->getPatientId();
    patient.nouveauPatient = this->isNewPatientSelected();

    type.typeMessage = 1;
    type.taille = sizeof(PATIENT);

    envoyerMessage(client, &type, sizeof(TYPE));
    envoyerMessage(client, &patient, sizeof(PATIENT));

    bool buffer;
    recevoirReponse(client, &buffer, sizeof(buffer));
    int id_pat;
    if(buffer > 0){
        recevoirReponse(client, &id_pat, sizeof(int));
        loginOk();
        this->setPatientId(id_pat);
    }
}

void MainWindowClientConsultationBooker::on_pushButtonLogout_clicked()
{
    TYPE type;
    type.typeMessage = 3;
    envoyerMessage(client, &type, sizeof(TYPE));
    bool buffer;
    recevoirReponse(client, &buffer, sizeof(buffer));
    if(buffer > 0){
        logoutOk();
    }
}

void MainWindowClientConsultationBooker::on_pushButtonRechercher_clicked()
{
    this->clearTableConsultations();
    RECHERCHE recherche;
    strncpy(recherche.nomSpecialite, this->getSelectionSpecialty().c_str(), sizeof(recherche.nomSpecialite)-1);
    recherche.nomSpecialite[sizeof(recherche.nomSpecialite)-1] = '\0';


    string doctor = this->getSelectionDoctor();
    strncpy(recherche.nom, doctor.c_str(), sizeof(recherche.nom)-1);
    recherche.nom[sizeof(recherche.nom)-1] = '\0';


    string startDate = this->getStartDate();
    strncpy(recherche.dateDebut, startDate.c_str(), sizeof(recherche.dateDebut)-1);
    recherche.dateDebut[sizeof(recherche.dateDebut)-1] = '\0';


    string endDate = this->getEndDate();
    strncpy(recherche.dateFin, endDate.c_str(), sizeof(recherche.dateFin)-1);
    recherche.dateFin[sizeof(recherche.dateFin)-1] = '\0';

    TYPE type;
    type.typeMessage = 2;
    type.taille = sizeof(RECHERCHE);

    envoyerMessage(client, &type, sizeof(TYPE));


    envoyerMessage(client, &recherche, sizeof(RECHERCHE));


    int nbResultats;
    recevoirReponse(client, &nbResultats, sizeof(nbResultats));

    printf("Nombre de résultats reçus: %d\n", nbResultats);


    if(nbResultats > 0) {
        recevoirReponse(client, tabReponses.data(), nbResultats * sizeof(REPONSE_RECHERCHE));
        char nomComplet[50];
        for(int i = 0; i < nbResultats; i++)
        {
            printf("Consultation %d: Dr %s %s - %s à %s\n", 
            tabReponses[i].idConsultation, 
            tabReponses[i].prenomMedecin, 
            tabReponses[i].nomMedecin,
            tabReponses[i].dateConsultation, 
            tabReponses[i].hourConsultation);
            sprintf(nomComplet, "%s %s", tabReponses[i].nomMedecin, tabReponses[i].prenomMedecin);
            this->addTupleTableConsultations(tabReponses[i].idConsultation, 
                            tabReponses[i].nomSpecialite,
                            nomComplet,
                            tabReponses[i].dateConsultation, 
                            tabReponses[i].hourConsultation);
                cout << "specialty = " << tabReponses[i].nomSpecialite << endl;
                cout << "doctor = " << nomComplet << endl;
                cout << "date = " << tabReponses[i].dateConsultation << endl;
                cout << "hour = " << tabReponses[i].hourConsultation << endl;
                cout << endl;
                cout << endl;
                cout << endl;
                cout << endl;
        }
    }
}

void MainWindowClientConsultationBooker::on_pushButtonReserver_clicked()
{
    bool ok;
    QString texte = QInputDialog::getText(
        qobject_cast<QWidget*>(parent()),
        "La raison de votre consultation",
        "Entrez la raison :",
        QLineEdit::Normal,
        "",
        &ok
    );

    TYPE type;
    type.typeMessage = 6;
    type.taille = sizeof(BOOK_CONSULTATION);

    envoyerMessage(client, &type, sizeof(TYPE));
    int selectedTow = this->getSelectionIndexTableConsultations();
    BOOK_CONSULTATION BookCons;
    BookCons.id_consultation = tabReponses[selectedTow].idConsultation;
    BookCons.id_patient = this->getPatientId();
    strncpy(BookCons.raison_consultation, texte.toUtf8().constData(), sizeof(BookCons.raison_consultation) - 1);
    BookCons.raison_consultation[sizeof(BookCons.raison_consultation) - 1] = '\0';  
    envoyerMessage(client, &BookCons, sizeof(BOOK_CONSULTATION));
    bool result;
    recevoirReponse(client, &result, sizeof(bool));
    printf("résultat de la reservation : %d\n", result);
}
