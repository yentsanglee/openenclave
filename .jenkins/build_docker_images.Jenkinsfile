@Library("OpenEnclaveCommon") _
oe = new jenkins.common.Openenclave()

GLOBAL_TIMEOUT_MINUTES = 240

OETOOLS_REPO = "https://oejenkinscidockerregistry.azurecr.io"
OETOOLS_REPO_CREDENTIAL_ID = "oejenkinscidockerregistry"
OETOOLS_DOCKERHUB_REPO_CREDENTIAL_ID = "oeciteamdockerhub"

def buildDockerImages() {
parallel {
    stage("Linux Docker Images") {
        node("nonSGX") {
            timeout(GLOBAL_TIMEOUT_MINUTES) {
                stage("Checkout") {
                    cleanWs()
                    checkout scm
                }
            }
        }
    }
    stage("Windows Docker Images") {
        node('SGXFLC-Windows') {
            stage("Checkout") {
                cleanWs()
                checkout scm
            }
            oefullWin2016 = oe.dockerImage("oetools-full-ltsc2016:${DOCKER_TAG}", ".jenkins/Dockerfile.full.Windows", "--build-arg windows_version=ltsc2016")
            puboefullWin2016 = oe.dockerImage("oeciteam/oetools-full-ltsc2016:${DOCKER_TAG}", ".jenkins/Dockerfile.full.Windows", "--build-arg windows_version=ltsc2016")
            oefullWin2016.push()
            if(TAG_LATEST == "true") {
                oefullWin2016.push('latest')
            }
            docker.withRegistry('', OETOOLS_DOCKERHUB_REPO_CREDENTIAL_ID) {
                if(TAG_LATEST == "true") {
                        puboefullWin2016.push('latest')
                }
            }
        }
    }
}
}

buildDockerImages()
