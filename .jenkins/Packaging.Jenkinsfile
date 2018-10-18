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
            git config core.sshCommand "ssh -i $oeciteam_rsa -F /dev/null"
            sh 'bash ./scripts/deploy-docs build ssh'
          }   
        
        }
      }
    }
  }
}
}
