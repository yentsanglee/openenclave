pipeline {
  agent any
  stages {
stage('Build, Test, and Package') {
      parallel {
        stage('SGX1FLC Package Debug') {
          agent {
            node {
              label 'hardware'
          }

          }
          steps {
withCredentials([file(credentialsId: 'oeciteam_rsa', variable: 'oeciteam-rsa'),
                 file(credentialsId: 'oeciteam_rsa_pub', variable: 'oeciteam-rsa-pub')]) {
            git config core.sshCommand "ssh -i $oeciteam-rsa -F /dev/null"
            sh 'bash ./scripts/deploy-docs build ssh'
          }   
        
        }
      }
    }
  }
}
}
